#pragma once

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

    void addCommand(std::shared_ptr<ICommand> command);
    void executeCurrentCommand(int coreId);
    void moveToNextLine();
    bool hasFinished() const;

    // Accessors
    int getPID() const;
    int getProgramCounter() const;
    const std::string& getName() const;
    ProcessState getState() const;

    void setState(ProcessState state);
    void setProgramCounter(int pc);


private:
    int pid_;
    std::string name_;
    ProcessState state_;
    int programCounter_;

    std::vector<std::shared_ptr<ICommand>> commandList;
    SymbolTable symbolTable;
};