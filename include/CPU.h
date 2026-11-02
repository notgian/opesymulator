#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
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

    // Called when a process executes SLEEP: the core is freed (process is NOT terminated)
    // and the callback receives the process so it can be parked until it wakes up.
    using SleepCallback = std::function<void(std::shared_ptr<ProcessControlBlock>)>;
    void setOnSleep(SleepCallback callback);

    // Round robin support. The quantum is the number of CPU ticks a process may hold a
    // core before it is preempted. 0 means "no preemption" (FCFS: run until done).
    void setQuantum(std::uint64_t quantumCycles);

    // Called when a process used up its quantum: the core is freed, the process goes back
    // to READY and the callback re-queues it at the back of the ready queue.
    using PreemptCallback = std::function<void(std::shared_ptr<ProcessControlBlock>)>;
    void setOnPreempt(PreemptCallback callback);

    // Accessors (thread-safe)
    int getNumCores() const;
    bool isCoreBusy(int coreId) const;
    std::vector<RunningEntry> getRunningProcesses() const;   // sorted by core id

private:
    void workerLoop(int coreId);
    // Frees a core without terminating its process; the process is left in `newState`
    void releaseCore(int coreId, ProcessState newState);

    std::vector<CPUCore> cores_;
    std::vector<std::thread> workers_;

    std::uint64_t execDelayCycles_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stopping_{false};
    SleepCallback onSleep_;
    PreemptCallback onPreempt_;
    std::atomic<std::uint64_t> quantumCycles_{0};
};
