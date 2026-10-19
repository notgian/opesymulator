#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "ICommand.h"
#include "SymbolTable.h"

enum class ProcessState {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
};

class ProcessControlBlock {
public:
    ProcessControlBlock(int pid, const std::string& name);
    virtual ~ProcessControlBlock() = default;

    void addCommand(std::shared_ptr<ICommand> command);
    void executeCurrentCommand(int coreId);
    void moveToNextLine();
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


private:
    int pid_;
    std::string name_;
    std::string creationTime_;

    // Atomic: written by a CPU worker thread, read by the UI thread (screen -ls)
    std::atomic<ProcessState> state_;
    std::atomic<int> programCounter_;

    std::vector<std::shared_ptr<ICommand>> commandList;
    SymbolTable symbolTable;
};
