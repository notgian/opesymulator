#pragma once
 
#include <cstdint>
#include <string>
#include "Scheduler.h"

constexpr std::uint32_t MIN_MEM_RANGE = 64u;    // 2^6
constexpr std::uint32_t MAX_MEM_RANGE = 65536u; // 2^16
 
// Outcome of Config::load().
enum class ConfigLoadStatus {
    Success,          // File parsed, every attribute present and valid
    FileNotFound,     // File could not be opened
    MissingAttribute, // A required attribute was not found in the file
    InvalidValue,     // An attribute had an unexpected or out-of-range value
    Reinitialization  // Config has already been initialized
};

// Returns true if val is a power of 2 within [2^6, 2^16]
constexpr bool isValidMemoryPowerOfTwo(std::uint64_t val) {
    return val >= MIN_MEM_RANGE &&
           val <= MAX_MEM_RANGE &&
           (val & (val - 1u)) == 0u;
}
 
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

    // New getters for MO2 memory parameters.
    std::uint64_t getMaxMemOverall() const { return maxMemOverall_; }
    std::uint64_t getMemPerFrame() const { return memPerFrame_; }
    std::uint64_t getMinMemPerProc() const { return minMemPerProc_; }
    std::uint64_t getMaxMemPerProc() const { return maxMemPerProc_; }
 
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

    // new config values for MO2
    std::uint64_t maxMemOverall_ = MAX_MEM_RANGE;
    std::uint64_t memPerFrame_ = MIN_MEM_RANGE;
    std::uint64_t minMemPerProc_ = MIN_MEM_RANGE;
    std::uint64_t maxMemPerProc_ = MAX_MEM_RANGE;

    bool initialized_ = false;
};