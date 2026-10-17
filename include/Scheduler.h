#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include "ProcessControlBlock.h"
#include "CPU.h"

enum class SchedulerMode {
    FCFS
};

class Scheduler {
public:
    Scheduler();
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

    // Specific algorithm implementations
    void FCFCSchedule(CPU& cpu);
};
