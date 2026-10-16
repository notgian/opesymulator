#include "Scheduler.h"

Scheduler::Scheduler() : currentMode_(SchedulerMode::FCFS) {}

void Scheduler::setMode(SchedulerMode mode) {
    currentMode_ = mode;
}

SchedulerMode Scheduler::getMode() const {
    return currentMode_;
}

void Scheduler::addProcess(std::shared_ptr<ProcessControlBlock> pcb) {
    pcb->setState(ProcessState::READY);
    readyQueue_.push(pcb);
}

void Scheduler::update(CPU& cpu) {
    // Check the active mode and dispatch to the correct algorithm
    switch (currentMode_) {
        case SchedulerMode::FCFS:
            FCFCSchedule(cpu);
            break;
        default:
            break;
    }
}

void Scheduler::FCFCSchedule(CPU& cpu) {
    // Check available CPU cores and feed processes from the ready queue
    for (int i = 0; i < cpu.getNumCores(); ++i) {
        if (!cpu.isCoreBusy(i) && !readyQueue_.empty()) {
            auto nextProcess = readyQueue_.front();
            readyQueue_.pop();

            // Assign it to the idle core (internally switches its state to RUNNING)
            cpu.assignProcessToCore(i, nextProcess);
        }
    }
}