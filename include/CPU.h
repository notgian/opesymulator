#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdint>
#include "ProcessControlBlock.h"

constexpr int CYCLE_DURATION_MS = 100;

struct CPUCore {
    int coreId;
    bool isBusy = false;
    std::shared_ptr<ProcessControlBlock> currentProcess = nullptr;
};

// One entry of the "running processes" snapshot used by screen -ls
struct RunningEntry {
    int coreId;
    std::shared_ptr<ProcessControlBlock> process;
};

// The CPU owns one worker thread per core. Each worker sleeps until the
// scheduler assigns a process to its core, runs that process to completion
// (FCFS = non-preemptive), then releases the core.
class CPU {
public:
    CPU(int numCores, std::uint64_t execDelayCycles);
    ~CPU();

    void start();   // spawn one worker thread per core
    void stop();    // signal workers to quit and join them

    // Called by the scheduler thread
    bool assignProcessToCore(int coreId, std::shared_ptr<ProcessControlBlock> pcb);
    void clearCore(int coreId);

    // Accessors (thread-safe)
    int getNumCores() const;
    bool isCoreBusy(int coreId) const;
    std::vector<RunningEntry> getRunningProcesses() const;   // sorted by core id

private:
    void workerLoop(int coreId);

    std::vector<CPUCore> cores_;
    std::vector<std::thread> workers_;

    std::uint64_t execDelayCycles_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stopping_{false};
};
