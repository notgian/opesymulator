#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class MemoryManager;
struct PageTableEntry;

class SymbolTable {
public:
    static constexpr std::size_t MAX_VARIABLES = 32;
    static constexpr std::size_t VARIABLE_SIZE_BYTES = 2;
    static constexpr std::size_t SEGMENT_SIZE_BYTES = MAX_VARIABLES * VARIABLE_SIZE_BYTES; // 64 bytes
    static constexpr std::size_t SYMBOL_TABLE_PAGE = 0;

    SymbolTable() = default;
    SymbolTable(int pid, MemoryManager* memoryManager, std::vector<PageTableEntry>* pageTable);

    void bindMemory(int pid, MemoryManager* memoryManager, std::vector<PageTableEntry>* pageTable);

    // Declares or updates a uint16 variable in the 64-byte symbol table segment.
    // Ignores new variable declarations once the 32-variable limit is reached.
    bool setVariable(const std::string& name, int value);

    // Reads a uint16 variable from the symbol table segment (returns 0 if undeclared).
    std::uint16_t getVariable(const std::string& name);

    bool hasVariable(const std::string& name) const;
    std::size_t getVariableCount() const;
    bool isFull() const;

private:
    int ensureSymbolTablePageResident(bool isWrite);

    int pid_ = -1;
    MemoryManager* memoryManager_ = nullptr;
    std::vector<PageTableEntry>* pageTable_ = nullptr;

    // Maps variable name -> slot index [0, 31] (byte offset = slot * 2 in Virtual Page 0)
    std::unordered_map<std::string, std::size_t> varToSlot_;

    // Fallback storage if SymbolTable is constructed standalone without a MemoryManager
    std::array<std::uint16_t, MAX_VARIABLES> fallbackSlots_{};
};
