#pragma once

#include "SymbolTable.h"

enum class ICommandType {
    IO,
    PRINT
};

class ICommand {
public:
    ICommand(int pid, ICommandType CommandType);
    virtual ~ICommand() = default;
    
    ICommandType getCommandType() const;
    
    virtual void execute(SymbolTable& symbolTable) = 0;

protected:
    int pid_;
    ICommandType commandType_;
};