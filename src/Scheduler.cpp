#include "Scheduler.h"
#include <chrono>

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

void Scheduler::start(CPU& cpu) {
    if (thread_.joinable()) return;
    stopping_ = false;
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
    // first idle core. Processes are never preempted.
    for (int i = 0; i < cpu.getNumCores(); ++i) {
        if (cpu.isCoreBusy(i)) continue;

        std::shared_ptr<ProcessControlBlock> nextProcess;
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (readyQueue_.empty()) return;
            nextProcess = readyQueue_.front();
            readyQueue_.pop();
        }
        cpu.assignProcessToCore(i, nextProcess);
    }
}

// TODO: implement RR scheduler
void Scheduler::RRSchedule(CPU& cpu) {

}
