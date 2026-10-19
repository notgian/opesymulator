#pragma once

#include "ICommand.h"
#include "SymbolTable.h"

#include <string>

class PrintCommand : public ICommand {
public:
    PrintCommand(int pid, const std::string& message);

    void execute(SymbolTable& symbolTable) override;

private:
    std::string message_;
};
