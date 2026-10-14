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
    void executeCurrentCommand();
    void moveToNextLine();

    // Accessors
    int getPID() const;
    const std::string& getName() const;
    ProcessState getState() const;


private:
    int pid_;
    std::string name_;
    ProcessState state_;
    int programCounter_;

    std::vector<std::shared_ptr<ICommand>> commandList;
    SymbolTable symbolTable;
};