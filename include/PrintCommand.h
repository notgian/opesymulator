#pragma once

#include "ICommand.h"
#include "SymbolTable.h"

#include <string>

class PrintCommand : public ICommand {
public:
    PrintCommand(int pid, const std::string& message);

    void execute(SymbolTable& symbolTable) override;

    // Setter so the CPU/PCB can inject which core executed this instruction
    void setCoreId(int coreId);

private:
    std::string message_;
    int coreId_;
};