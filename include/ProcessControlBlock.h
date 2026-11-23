#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "ICommand.h"
#include "MemoryManager.h"
#include "SymbolTable.h"

enum class ProcessState {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

class ProcessControlBlock {
public:
    ProcessControlBlock(int pid,
                        const std::string& name,
                        std::size_t allocatedMemoryBytes = 64,
                        MemoryManager* memoryManager = nullptr);
    virtual ~ProcessControlBlock() = default;

    void addCommand(std::shared_ptr<ICommand> command);
    int executeCurrentCommand(int coreId);
    void jumpLines(int jump);
    bool hasFinished() const;

    // Accessors
    int getPID() const;
    int getProgramCounter() const;
    int getTotalCommands() const;
    const std::string& getCreationTime() const;
    const std::string& getName() const;
    ProcessState getState() const;

    void setState(ProcessState state);
    void setProgramCounter(int pc);

    // Core ID tracking for PRINT logs without consuming a SymbolTable variable slot
    void setCurrentCoreId(int coreId);
    int getCurrentCoreId() const;

    // Memory & Page Table accessors
    std::size_t getAllocatedMemory() const;
    std::size_t getNumPages() const;
    std::vector<PageTableEntry>& getPageTable();
    const std::vector<PageTableEntry>& getPageTable() const;
    SymbolTable& getSymbolTable();

    // Called by MemoryManager when one of this process's pages is evicted to the backing store
    void onPageEvicted(std::size_t virtualPage, std::size_t backingStoreLine);

    // Releases all physical frames and backing store slots owned by this process upon termination
    void releaseMemory();

    // SLEEP support: ticks the process must stay off the CPU (0 = not sleeping)
    void setSleepTicks(int ticks);
    int getSleepTicks() const;
    void clearSleep();

private:
    int pid_;
    std::string name_;
    std::string creationTime_;

    std::size_t allocatedMemoryBytes_;
    MemoryManager* memoryManager_ = nullptr;
    std::vector<PageTableEntry> pageTable_;

    // Atomic: written by a CPU worker thread, read by the UI thread (screen -ls)
    std::atomic<ProcessState> state_;
    std::atomic<int> programCounter_;
    std::atomic<int> sleepTicks_{0};
    std::atomic<int> currentCoreId_{-1};

    std::vector<std::shared_ptr<ICommand>> commandList;
    SymbolTable symbolTable;
};
