#include "CPU.h"
#include "Config.h"
#include <chrono>

CPU::CPU(int numCores, std::uint64_t execDelayCycles)
    : execDelayCycles_(execDelayCycles) {
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

// Worker thread: one per core
void CPU::workerLoop(int coreId) {
    while (true) {
        std::shared_ptr<ProcessControlBlock> pcb;

        {   // Wait until the scheduler hands this core a process
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [&] { return stopping_ || cores_[coreId].currentProcess != nullptr; });
            if (stopping_) return;
            pcb = cores_[coreId].currentProcess;
        }

        std::uint64_t cyclesSinceLastExecution = execDelayCycles_;

        // Run the process until it finishes (non-preemptive, as FCFS requires)
        while (!stopping_ && !pcb->hasFinished()) {
            if (cyclesSinceLastExecution < execDelayCycles_) {
                ++cyclesSinceLastExecution;
            } else {
                int jump = pcb->executeCurrentCommand(coreId);
                pcb->jumpLines(jump);
                cyclesSinceLastExecution = 0;

                if (pcb->hasFinished()) break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_DURATION_MS));
        }
        if (stopping_) return;

        // Mark finished and free the core in one atomic step so a "screen -ls"
        // snapshot never sees the process as both running and finished.
        clearCore(coreId);
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
