#pragma once

// ---------------------------------------------------------------------------
// OLD!! DEPRECEATED!! Central emulator configuration
// ---------------------------------------------------------------------------

// 1 = every process writes its print commands to logs/<process>.txt
//     (needed ONLY for the recorded homework test case).
// 0 = no text files are created  -> set this to 0 for the machine project
//     submission, because file I/O hurts scheduler performance.
#define ENABLE_PRINT_LOGS 1

constexpr int NUM_CORES           = 4;    // required by the assignment
constexpr int NUM_TEST_PROCESSES  = 10;   // test case: 10 processes
constexpr int PRINTS_PER_PROCESS  = 100;  // test case: 100 print commands each

// Simulated time one instruction takes on a core. 100 ms -> each process runs
// ~10 s, so the whole test lasts ~30 s and "screen -ls" can be sampled many times.
constexpr int INSTRUCTION_DELAY_MS = 100;

// ---------------------------------------------------------------------------
// OLD!! DEPRECEATED!! END
// ---------------------------------------------------------------------------
 
#include <cstdint>
#include <string>
#include "Scheduler.h"
 
// Outcome of Config::load().
enum class ConfigLoadStatus {
    Success,          // File parsed, every attribute present and valid
    FileNotFound,     // File could not be opened
    MissingAttribute, // A required attribute was not found in the file
    InvalidValue,     // An attribute had an unexpected or out-of-range value
    Reinitialization  // Config has already been initialized
};
 
class Config {
public:
    static constexpr const char* DEFAULT_PATH = "./config.txt";
 
    // Loads the configuration from the given file. On any failure the
    // existing values are left untouched (no partial updates).
    ConfigLoadStatus load(const std::string& path = DEFAULT_PATH);
 
    // Getters (no setters: values can only change through load()).
    std::uint32_t getNumCpu() const { return numCpu_; }
    SchedulerMode getScheduler() const { return scheduler_; }
    std::uint64_t getQuantumCycles() const { return quantumCycles_; }
    std::uint64_t getBatchProcessFreq() const { return batchProcessFreq_; }
    std::uint64_t getMinIns() const { return minIns_; }
    std::uint64_t getMaxIns() const { return maxIns_; }
    std::uint64_t getDelaysPerExec() const { return delaysPerExec_; }
 
private:
    // Initialized to the default parameters from the specification.
    // Ranges go up to 2^32 inclusive, so 64-bit storage is used.
    std::uint8_t numCpu_ = 4;
    SchedulerMode scheduler_ = SchedulerMode::RR;
    std::uint64_t quantumCycles_ = 5;
    std::uint64_t batchProcessFreq_ = 1;
    std::uint64_t minIns_ = 1000;
    std::uint64_t maxIns_ = 2000;
    std::uint64_t delaysPerExec_ = 0;

    bool initialized_ = false;
};