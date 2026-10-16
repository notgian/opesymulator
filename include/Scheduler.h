#pragma once

#include <queue>
#include <memory>
#include "ProcessControlBlock.h"
#include "CPU.h"

enum class SchedulerMode {
    FCFS
};

class Scheduler {
public:
    Scheduler();

    void setMode(SchedulerMode mode);
    SchedulerMode getMode() const;

    void addProcess(std::shared_ptr<ProcessControlBlock> pcb);

    // Main update func called by kernel
    void update(CPU& cpu);

private:
    SchedulerMode currentMode_;
    std::queue<std::shared_ptr<ProcessControlBlock>> readyQueue_;

    // Specific algorithm implementations
    void FCFCSchedule(CPU& cpu);
};