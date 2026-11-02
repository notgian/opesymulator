#pragma once

#include "ICommand.h"
#include "SymbolTable.h"
#include <cstdint>

// SLEEP(X) - sleeps the current process for X (uint8) CPU ticks and relinquishes the CPU
class SleepCommand : public ICommand {
public:
    SleepCommand(int pid, std::uint8_t ticks);

    int execute(SymbolTable& symbolTable) override;

private:
    std::uint8_t ticks_;
};
