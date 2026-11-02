#pragma once

#include "ICommand.h"
#include "Operand.h"
#include "SymbolTable.h"
#include <string>

// SUBTRACT(var1, var2/value, var3/value) - var1 = var2/value - var3/value (clamped to uint16)
class SubtractCommand : public ICommand {
public:
    SubtractCommand(int pid, const std::string& dest, const Operand& lhs, const Operand& rhs);

    int execute(SymbolTable& symbolTable) override;

private:
    std::string dest_;
    Operand lhs_;
    Operand rhs_;
};
