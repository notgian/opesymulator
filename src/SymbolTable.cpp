#include "SymbolTable.h"
#include "MemoryManager.h"
#include "Operand.h"
#include "PhysicalMemory.h"
#include <stdexcept>

SymbolTable::SymbolTable(int pid, MemoryManager* memoryManager, std::vector<PageTableEntry>* pageTable) {
    bindMemory(pid, memoryManager, pageTable);
}

void SymbolTable::bindMemory(int pid, MemoryManager* memoryManager, std::vector<PageTableEntry>* pageTable) {
    pid_ = pid;
    memoryManager_ = memoryManager;
    pageTable_ = pageTable;
    varToSlot_.clear();
    fallbackSlots_.fill(0);
}

int SymbolTable::ensureSymbolTablePageResident(bool isWrite) {
    if (!memoryManager_ || !pageTable_) {
        return -1;
    }
    if (pageTable_->empty()) {
        throw std::out_of_range("Process page table has no symbol table page.");
    }

    int frameIdx = -1;
    while (frameIdx < 0) {
        frameIdx = memoryManager_->ensurePageResident(
            pid_,
            SYMBOL_TABLE_PAGE,
            (*pageTable_)[SYMBOL_TABLE_PAGE],
            isWrite
        );
    }
    return frameIdx;
}

bool SymbolTable::setVariable(const std::string& name, int value) {
    const std::uint16_t clamped = static_cast<std::uint16_t>(clampUint16(value));

    // Ensure Virtual Page 0 (the 64-byte symbol table segment) is loaded into PhysicalMemory
    const int frameIdx = ensureSymbolTablePageResident(/*isWrite=*/true);

    auto it = varToSlot_.find(name);
    if (it != varToSlot_.end()) {
        const std::size_t slot = it->second;
        if (frameIdx >= 0 && memoryManager_) {
            memoryManager_->getPhysicalMemory().writeUint16(
                static_cast<std::size_t>(frameIdx),
                slot * VARIABLE_SIZE_BYTES,
                clamped
            );
        } else {
            fallbackSlots_[slot] = clamped;
        }
        return true;
    }

    // Enforce the 32-variable (64-byte) cap: ignore new declarations once full
    if (varToSlot_.size() >= MAX_VARIABLES) {
        return false;
    }

    const std::size_t newSlot = varToSlot_.size();
    varToSlot_[name] = newSlot;

    if (frameIdx >= 0 && memoryManager_) {
        memoryManager_->getPhysicalMemory().writeUint16(
            static_cast<std::size_t>(frameIdx),
            newSlot * VARIABLE_SIZE_BYTES,
            clamped
        );
    } else {
        fallbackSlots_[newSlot] = clamped;
    }
    return true;
}

std::uint16_t SymbolTable::getVariable(const std::string& name) {
    // Ensure Virtual Page 0 (symbol table segment) is resident before reading
    const int frameIdx = ensureSymbolTablePageResident(/*isWrite=*/false);

    auto it = varToSlot_.find(name);
    if (it == varToSlot_.end()) {
        return 0;
    }

    const std::size_t slot = it->second;
    if (frameIdx >= 0 && memoryManager_) {
        return memoryManager_->getPhysicalMemory().readUint16(
            static_cast<std::size_t>(frameIdx),
            slot * VARIABLE_SIZE_BYTES
        );
    }
    return fallbackSlots_[slot];
}

bool SymbolTable::hasVariable(const std::string& name) const {
    return varToSlot_.find(name) != varToSlot_.end();
}

std::size_t SymbolTable::getVariableCount() const {
    return varToSlot_.size();
}

bool SymbolTable::isFull() const {
    return varToSlot_.size() >= MAX_VARIABLES;
}
