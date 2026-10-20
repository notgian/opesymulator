#include "Config.h"

#include <fstream>
#include <sstream>

namespace {

constexpr std::uint64_t MAX_2_POW_32 = 4294967296ULL; // 2^32

// Bit flags used to track which required attributes have been seen.
enum Attr : unsigned {
    ATTR_NUM_CPU = 1u << 0,
    ATTR_SCHEDULER = 1u << 1,
    ATTR_QUANTUM_CYCLES = 1u << 2,
    ATTR_BATCH_FREQ = 1u << 3,
    ATTR_MIN_INS = 1u << 4,
    ATTR_MAX_INS = 1u << 5,
    ATTR_DELAYS = 1u << 6,
    ATTR_ALL = (1u << 7) - 1
};

// Parses a non-negative decimal integer in [min, max]. Rejects empty
// strings, signs, whitespace, trailing characters, and overflow.
bool parseUInt(const std::string& s, std::uint64_t min, std::uint64_t max,
               std::uint64_t& out) {
    if (s.empty()) return false;
    std::uint64_t value = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + static_cast<std::uint64_t>(c - '0');
        if (value > max) return false; // also prevents 64-bit overflow
    }
    if (value < min) return false;
    out = value;
    return true;
}

// Accepts "fcfs" or "rr", with or without surrounding double quotes.
bool parseScheduler(std::string s, std::string& out) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }
    if (s == "fcfs" || s == "rr") {
        out = s;
        return true;
    }
    return false;
}

}

ConfigLoadStatus Config::load(const std::string& path) {
    if (initialized_)
        return ConfigLoadStatus::Reinitialization;

    std::ifstream file(path);
    if (!file.is_open()) {
        return ConfigLoadStatus::FileNotFound;
    }

    // Parse into temporaries so a failed load never modifies this object.
    std::uint64_t numCpu = 0, quantum = 0, batchFreq = 0;
    std::uint64_t minIns = 0, maxIns = 0, delays = 0;
    std::string scheduler;
    unsigned seen = 0;

    std::string line;
    bool firstLine = true;
    while (std::getline(file, line)) {
        // Strip a UTF-8 BOM if the file was saved with one (e.g. Notepad).
        if (firstLine && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
            line.erase(0, 3);
        }
        firstLine = false;

        std::istringstream iss(line);
        std::string key, value, extra;
        if (!(iss >> key)) continue; // blank line

        const bool hasValue = static_cast<bool>(iss >> value);
        const bool hasExtra = hasValue && static_cast<bool>(iss >> extra);

        // Determine which attribute this is; unknown keys are ignored.
        unsigned attr = 0;
        if (key == "num-cpu") attr = ATTR_NUM_CPU;
        else if (key == "scheduler") attr = ATTR_SCHEDULER;
        else if (key == "quantum-cycles") attr = ATTR_QUANTUM_CYCLES;
        else if (key == "batch-process-freq") attr = ATTR_BATCH_FREQ;
        else if (key == "min-ins") attr = ATTR_MIN_INS;
        else if (key == "max-ins") attr = ATTR_MAX_INS;
        // The spec table says "delays-per-exec" but the sample file says
        // "delay-per-exec", so both spellings are accepted.
        else if (key == "delays-per-exec" || key == "delay-per-exec") attr = ATTR_DELAYS;
        else continue;

        // A recognized attribute must have exactly one value.
        if (!hasValue || hasExtra) return ConfigLoadStatus::InvalidValue;

        bool ok = false;
        switch (attr) {
            case ATTR_NUM_CPU:
                ok = parseUInt(value, 1, 128, numCpu);
                break;
            case ATTR_SCHEDULER:
                ok = parseScheduler(value, scheduler);
                break;
            case ATTR_QUANTUM_CYCLES:
                ok = parseUInt(value, 1, MAX_2_POW_32, quantum);
                break;
            case ATTR_BATCH_FREQ:
                ok = parseUInt(value, 1, MAX_2_POW_32, batchFreq);
                break;
            case ATTR_MIN_INS:
                ok = parseUInt(value, 1, MAX_2_POW_32, minIns);
                break;
            case ATTR_MAX_INS:
                ok = parseUInt(value, 1, MAX_2_POW_32, maxIns);
                break;
            case ATTR_DELAYS:
                ok = parseUInt(value, 0, MAX_2_POW_32, delays);
                break;
        }
        if (!ok) return ConfigLoadStatus::InvalidValue;

        seen |= attr;
    }

    if (seen != ATTR_ALL) {
        return ConfigLoadStatus::MissingAttribute;
    }

    // Cross-field check: a process cannot have a minimum above its maximum.
    if (minIns > maxIns) {
        return ConfigLoadStatus::InvalidValue;
    }

    // Everything is valid: commit.
    numCpu_ = static_cast<std::uint32_t>(numCpu);
    scheduler_ = scheduler;
    quantumCycles_ = quantum;
    batchProcessFreq_ = batchFreq;
    minIns_ = minIns;
    maxIns_ = maxIns;
    delaysPerExec_ = delays;

    initialized_ = true;
    return ConfigLoadStatus::Success;
}