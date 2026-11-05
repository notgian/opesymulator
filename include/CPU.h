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

// Forward declaration to prevent circular dependencies
class Kernel;

// One CPU tick = one frame pass of the scheduler thread (see the spec's CPU tick pseudocode).
// Everything that is measured in "CPU cycles" is counted in these ticks and nothing else:
//   - batch-process-freq (a new dummy process every N ticks)
//   - quantum-cycles     (a core may hold a process for N ticks)
//   - delay-per-exec     (busy-wait ticks between two instructions)
//   - SLEEP (X)          (the process sleeps for X ticks)
// Each core executes at most one tick of work per tick, so the config.txt values behave the
// same regardless of how fast the machine is. TICK_DURATION_MS is only the real time one tick
// takes; it does not change any result, just how fast the emulator runs on screen.
constexpr int TICK_DURATION_MS = 10;

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
    CPU(int numCores, std::uint64_t execDelayCycles, Kernel& kernel);
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

    // Lets a core take the next ready process itself the moment it frees up, instead of
    // sitting idle until the scheduler thread's next pass. Returns nullptr if none is ready.
    using PullCallback = std::function<std::shared_ptr<ProcessControlBlock>()>;
    void setPullNext(PullCallback callback);

    // Accessors (thread-safe)
    int getNumCores() const;
    bool isCoreBusy(int coreId) const;
    std::vector<RunningEntry> getRunningProcesses() const;   // sorted by core id

    void setGenerateDummyProcs(bool value);
    bool getGenerateDummyProcs();

    // Global CPU tick clock. The scheduler thread advances it once per frame pass; every core
    // worker waits for the next tick before executing one step.
    std::uint64_t advanceTick();
    std::uint64_t currentTick() const;

    friend class Scheduler;


private:
    void workerLoop(int coreId);
    // Frees a core without terminating its process; the process is left in `newState`
    void releaseCore(int coreId, ProcessState newState);

    // Ends the current turn on a core in ONE critical section: the process gets `newState`,
    // is re-queued (preempted) or parked (sleeping) if asked, and the core immediately pulls
    // the next ready process, if any.
    void finishTurn(int coreId, ProcessState newState, bool requeue = false, bool park = false);

    std::atomic<bool> generateDummyProcesses_{false};
    std::uint64_t batchProcessFreq_ ;

    std::vector<CPUCore> cores_;
    std::vector<std::thread> workers_;

    std::uint64_t execDelayCycles_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stopping_{false};

    std::atomic<std::uint64_t> tick_{0};
    std::mutex tickMutex_;
    std::condition_variable tickCv_;

    SleepCallback onSleep_;
    PreemptCallback onPreempt_;
    PullCallback pullNext_;
    std::atomic<std::uint64_t> quantumCycles_{0};

    Kernel& kernel_;
};
