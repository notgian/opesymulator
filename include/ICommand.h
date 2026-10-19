#pragma once

#include <string>
#include "SymbolTable.h"

class ProcessControlBlock;

enum class ICommandType {
    IO,
    PRINT
};

class ICommand {
public:
    ICommand(int pid, ICommandType CommandType);
    virtual ~ICommand() = default;
    
    virtual void execute(SymbolTable& symbolTable) = 0;

    ICommandType getCommandType() const;
    ProcessControlBlock* getParentProcess() const;
    void setParentProcess(ProcessControlBlock* parent);

protected:
    int pid_;
    ICommandType commandType_;
    ProcessControlBlock* parentProcess_;
};
