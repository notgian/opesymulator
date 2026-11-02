#pragma once

#include "ICommand.h"
#include "SymbolTable.h"
#include <string>

// DECLARE(var, value) - declares a uint16 variable with a default value
class DeclareCommand : public ICommand {
public:
    DeclareCommand(int pid, const std::string& var, int value);

    int execute(SymbolTable& symbolTable) override;

private:
    std::string var_;
    int value_;
};
