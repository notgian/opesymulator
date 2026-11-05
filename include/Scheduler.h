#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <chrono>
#include <queue>
#include <vector>
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

    // Takes the oldest ready process (nullptr if none). Used by cores to pick their own next job.
    std::shared_ptr<ProcessControlBlock> popReady();

    // Parks a process that executed SLEEP; it returns to the ready queue once its ticks elapse
    void addSleepingProcess(std::shared_ptr<ProcessControlBlock> pcb);

    // Scheduler thread: repeatedly dispatches ready processes to idle cores
    void start(CPU& cpu);
    void stop();

    // One scheduling pass (called in a loop by the scheduler thread)
    void update(CPU& cpu);

private:
    SchedulerMode currentMode_;

    std::mutex queueMutex_;
    std::queue<std::shared_ptr<ProcessControlBlock>> readyQueue_;

    struct SleepingEntry {
        std::shared_ptr<ProcessControlBlock> pcb;
        std::chrono::steady_clock::time_point wakeAt;
    };
    std::mutex sleepMutex_;
    std::vector<SleepingEntry> sleeping_;
    void wakeSleepingProcesses();

    std::thread thread_;
    std::atomic<bool> stopping_{false};

    std::uint64_t quantumCycles_;
    std::uint64_t globalTicks_ = 0;

    // Specific algorithm implementations
    void FCFCSchedule(CPU& cpu);
    void RRSchedule(CPU& cpu);

    // Shared by both algorithms: oldest ready process -> first idle core
    void dispatchToIdleCores(CPU& cpu);
};
