#include "CPU.h"
#include "Config.h"
#include "Kernel.h"
#include <chrono>

CPU::CPU(int numCores, std::uint64_t execDelayCycles, Kernel& kernel)
    : execDelayCycles_(execDelayCycles), kernel_(kernel) {
    for (int i = 0; i < numCores; ++i) {
        cores_.push_back({i, false, nullptr});
    }
}

CPU::~CPU() {
    stop();
}

void CPU::start() {
    if (!workers_.empty()) return;
    stopping_ = false;
    for (int i = 0; i < static_cast<int>(cores_.size()); ++i) {
        workers_.emplace_back(&CPU::workerLoop, this, i);
    }
}

void CPU::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    {   // make sure no worker misses the stop signal while waiting for a tick
        std::lock_guard<std::mutex> tickLock(tickMutex_);
    }
    tickCv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
}

bool CPU::assignProcessToCore(int coreId, std::shared_ptr<ProcessControlBlock> pcb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (coreId < 0 || coreId >= static_cast<int>(cores_.size())) return false;
        if (cores_[coreId].isBusy) return false;

        cores_[coreId].currentProcess = pcb;
        cores_[coreId].isBusy = true;
        pcb->setState(ProcessState::RUNNING);
    }
    cv_.notify_all();   // wake that core's worker thread
    return true;
}

void CPU::clearCore(int coreId) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (coreId >= 0 && coreId < static_cast<int>(cores_.size())) {
        if (cores_[coreId].currentProcess) {
            cores_[coreId].currentProcess->setState(ProcessState::TERMINATED);
        }
        cores_[coreId].currentProcess = nullptr;
        cores_[coreId].isBusy = false;
    }
}

void CPU::setOnSleep(SleepCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    onSleep_ = std::move(callback);
}

void CPU::setQuantum(std::uint64_t quantumCycles) {
    quantumCycles_.store(quantumCycles);
}

void CPU::setOnPreempt(PreemptCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    onPreempt_ = std::move(callback);
}

void CPU::setPullNext(PullCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    pullNext_ = std::move(callback);
}

// Lock order: mutex_ -> (scheduler queue / sleep mutex). Nothing takes them the other way round.
void CPU::finishTurn(int coreId, ProcessState newState, bool requeue, bool park) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (coreId < 0 || coreId >= static_cast<int>(cores_.size())) return;
    CPUCore& core = cores_[coreId];

    std::shared_ptr<ProcessControlBlock> done = core.currentProcess;
    core.currentProcess = nullptr;
    core.isBusy = false;

    if (done) {
        done->setState(newState);
        if (requeue && onPreempt_) onPreempt_(done);   // back of the ready queue
        else if (park && onSleep_) onSleep_(done);     // into the scheduler's sleep list
    }

    // Take the next ready process right away (no idle gap waiting for the scheduler thread)
    if (pullNext_) {
        std::shared_ptr<ProcessControlBlock> next = pullNext_();
        if (next) {
            core.currentProcess = next;
            core.isBusy = true;
            next->setState(ProcessState::RUNNING);
        }
    }
}

// Frees the core without marking the process TERMINATED (it is only WAITING or READY)
void CPU::releaseCore(int coreId, ProcessState newState) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (coreId >= 0 && coreId < static_cast<int>(cores_.size())) {
        if (cores_[coreId].currentProcess) {
            cores_[coreId].currentProcess->setState(newState);
        }
        cores_[coreId].currentProcess = nullptr;
        cores_[coreId].isBusy = false;
    }
}

// Worker thread: one per core
void CPU::workerLoop(int coreId) {
    std::uint64_t lastTick = tick_.load();   // last tick this core has already worked on

    while (true) {
        std::shared_ptr<ProcessControlBlock> pcb;

        {   // Wait until the scheduler hands this core a process
            std::unique_lock<std::mutex> lock(mutex_);
            const bool wasIdle = (cores_[coreId].currentProcess == nullptr);
            cv_.wait(lock, [&] { return stopping_ || cores_[coreId].currentProcess != nullptr; });
            if (stopping_) return;
            pcb = cores_[coreId].currentProcess;
            // An idle core did not work during the ticks that passed; do not "catch up" on them.
            if (wasIdle) lastTick = tick_.load();
        }

        std::uint64_t cyclesSinceLastExecution = execDelayCycles_;
        bool sleeping = false;
        bool preempted = false;
        std::uint64_t ticksUsed = 0;                        // ticks spent on this core in this turn
        const std::uint64_t quantum = quantumCycles_.load(); // 0 = FCFS, never preempt

        // Run the process until it finishes, sleeps, or (round robin) uses up its quantum
        while (!stopping_ && !pcb->hasFinished()) {
            // Lock-step with the CPU clock: one step of work per tick, never more.
            {
                std::unique_lock<std::mutex> tickLock(tickMutex_);
                tickCv_.wait(tickLock, [&] { return stopping_ || tick_.load() > lastTick; });
            }
            if (stopping_) break;
            ++lastTick;

            if (cyclesSinceLastExecution < execDelayCycles_) {
                ++cyclesSinceLastExecution;
            } else {
                int jump = pcb->executeCurrentCommand(coreId);
                pcb->jumpLines(jump);
                cyclesSinceLastExecution = 0;

                if (pcb->hasFinished()) break;

                // SLEEP was just executed: relinquish the core
                if (pcb->getSleepTicks() > 0) {
                    sleeping = true;
                    break;
                }
            }

            // One CPU tick has passed. Busy-wait (delays-per-exec) ticks count too, because
            // the process is still occupying the core.
            ++ticksUsed;

            if (quantum > 0 && ticksUsed >= quantum) {
                preempted = true;
                break;
            }
        }
        if (stopping_) return;

        if (preempted) {
            // Quantum expired and the process is not done: back of the ready queue,
            // and this core immediately takes the next ready process.
            finishTurn(coreId, ProcessState::READY, /*requeue=*/true);
            continue;
        }

        if (sleeping) {
            // SLEEP: free the core, park the process until its ticks elapse
            finishTurn(coreId, ProcessState::WAITING, /*requeue=*/false, /*park=*/true);
            continue;
        }

        // Finished. Mark TERMINATED and free the core in one atomic step so a "screen -ls"
        // snapshot never sees the process as both running and finished.
        finishTurn(coreId, ProcessState::TERMINATED);
    }
}

int CPU::getNumCores() const {
    return static_cast<int>(cores_.size());
}

bool CPU::isCoreBusy(int coreId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cores_.at(coreId).isBusy;
}

std::vector<RunningEntry> CPU::getRunningProcesses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RunningEntry> result;
    for (const auto& core : cores_) {
        if (core.isBusy && core.currentProcess) {
            result.push_back({core.coreId, core.currentProcess});
        }
    }
    return result;
}

std::uint64_t CPU::advanceTick() {
    std::uint64_t now;
    {
        std::lock_guard<std::mutex> lock(tickMutex_);
        now = ++tick_;
    }
    tickCv_.notify_all();
    return now;
}

std::uint64_t CPU::currentTick() const {
    return tick_.load();
}

void CPU::setGenerateDummyProcs(bool value) {
    generateDummyProcesses_ = value;
}

bool CPU::getGenerateDummyProcs() {
    return generateDummyProcesses_;
}