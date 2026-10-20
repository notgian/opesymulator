#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <cstdint>
#include "ProcessControlBlock.h"
#include "CPU.h"

enum class SchedulerMode {
    FCFS,
    RR
};

class Scheduler {
public:
    Scheduler(SchedulerMode schedulerMode, std::uint64_t quantumCycles);
    ~Scheduler();

    void setMode(SchedulerMode mode);
    SchedulerMode getMode() const;

    void addProcess(std::shared_ptr<ProcessControlBlock> pcb);

    // Scheduler thread: repeatedly dispatches ready processes to idle cores
    void start(CPU& cpu);
    void stop();

    // One scheduling pass (called in a loop by the scheduler thread)
    void update(CPU& cpu);

private:
    SchedulerMode currentMode_;

    std::mutex queueMutex_;
    std::queue<std::shared_ptr<ProcessControlBlock>> readyQueue_;

    std::thread thread_;
    std::atomic<bool> stopping_{false};

    std::uint64_t quantumCycles_;

    // Specific algorithm implementations
    void FCFCSchedule(CPU& cpu);
    void RRSchedule(CPU& cpu);
};
