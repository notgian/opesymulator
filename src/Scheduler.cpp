#include "Scheduler.h"
#include <algorithm>
#include <chrono>
#include "CPU.h"  
#include "Kernel.h"

Scheduler::Scheduler(SchedulerMode schedulerMode, std::uint64_t quantumCycles) : currentMode_(schedulerMode), quantumCycles_(quantumCycles) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::setMode(SchedulerMode mode) {
    currentMode_ = mode;
}

SchedulerMode Scheduler::getMode() const {
    return currentMode_;
}

void Scheduler::addProcess(std::shared_ptr<ProcessControlBlock> pcb) {
    pcb->setState(ProcessState::READY);
    std::lock_guard<std::mutex> lock(queueMutex_);
    readyQueue_.push(pcb);
}

std::shared_ptr<ProcessControlBlock> Scheduler::popReady() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (readyQueue_.empty()) return nullptr;
    std::shared_ptr<ProcessControlBlock> next = readyQueue_.front();
    readyQueue_.pop();
    return next;
}

void Scheduler::addSleepingProcess(std::shared_ptr<ProcessControlBlock> pcb) {
    // 1 CPU tick = CYCLE_DURATION_MS
    auto wakeAt = std::chrono::steady_clock::now() +
                  std::chrono::milliseconds(static_cast<long long>(pcb->getSleepTicks()) * CYCLE_DURATION_MS);
    std::lock_guard<std::mutex> lock(sleepMutex_);
    sleeping_.push_back({pcb, wakeAt});
}

void Scheduler::wakeSleepingProcesses() {
    std::vector<std::shared_ptr<ProcessControlBlock>> awake;
    {
        std::lock_guard<std::mutex> lock(sleepMutex_);
        auto now = std::chrono::steady_clock::now();
        for (auto it = sleeping_.begin(); it != sleeping_.end();) {
            if (it->wakeAt <= now) {
                awake.push_back(it->pcb);
                it = sleeping_.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& pcb : awake) {
        pcb->clearSleep();
        addProcess(pcb);   // back to the ready queue
    }
}

void Scheduler::start(CPU& cpu) {
    if (thread_.joinable()) return;
    stopping_ = false;

    // FCFS never preempts (quantum 0); round robin preempts after quantum-cycles ticks.
    // A quantum below 1 would starve the process, so RR uses at least 1.
    cpu.setQuantum(currentMode_ == SchedulerMode::RR ? std::max<std::uint64_t>(1, quantumCycles_) : 0);
    thread_ = std::thread([this, &cpu] {
        while (!stopping_) {
            update(cpu);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
}

void Scheduler::stop() {
    stopping_ = true;
    if (thread_.joinable()) thread_.join();
}

void Scheduler::update(CPU& cpu) {
    wakeSleepingProcesses();

    globalTicks_++;


    bool generateDummyProcs = cpu.getGenerateDummyProcs();
    std::uint64_t batchProcFreq = cpu.kernel_.getConfig().getBatchProcessFreq();

    if (generateDummyProcs && globalTicks_ % batchProcFreq == 0) {
        cpu.kernel_.spawnDummyProcess();
    }


    // Check the active mode and dispatch to the correct algorithm
    switch (currentMode_) {
        case SchedulerMode::FCFS:
            FCFCSchedule(cpu);
            break;
        case SchedulerMode::RR:
            RRSchedule(cpu);
            break;
        default:
            break;
    }
}

void Scheduler::FCFCSchedule(CPU& cpu) {
    // First-come-first-serve: the oldest process in the ready queue goes to the
    // first idle core. The CPU never preempts (quantum 0), so it runs to completion.
    dispatchToIdleCores(cpu);
}

void Scheduler::RRSchedule(CPU& cpu) {
    // Round robin uses the same dispatch order as FCFS. The difference is that the CPU
    // preempts a process after `quantum` ticks and hands it back through addProcess(),
    // which puts it at the back of the queue, so everyone gets a turn.
    dispatchToIdleCores(cpu);
}

void Scheduler::dispatchToIdleCores(CPU& cpu) {
    for (int i = 0; i < cpu.getNumCores(); ++i) {
        if (cpu.isCoreBusy(i)) continue;

        std::shared_ptr<ProcessControlBlock> nextProcess;
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (readyQueue_.empty()) return;
            nextProcess = readyQueue_.front();
            readyQueue_.pop();
        }
        // A core may have pulled its own work since the isCoreBusy() check above; if so,
        // put the process back instead of losing it.
        if (!cpu.assignProcessToCore(i, nextProcess)) {
            addProcess(nextProcess);
        }
    }
}
