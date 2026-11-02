#pragma once

#include "ICommand.h"
#include "SymbolTable.h"

#include <string>

class PrintCommand : public ICommand {
public:
    PrintCommand(int pid, const std::string& message);
    // PRINT("msg" + var): appends the current value of `var` to the message
    PrintCommand(int pid, const std::string& message, const std::string& varName);

    int execute(SymbolTable& symbolTable) override;

private:
    std::string message_;
    std::string varName_;   // empty = no variable
};