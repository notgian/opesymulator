#pragma once
 
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