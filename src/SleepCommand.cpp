#include "SleepCommand.h"
#include "ProcessControlBlock.h"

SleepCommand::SleepCommand(int pid, std::uint8_t ticks)
    : ICommand(pid, ICommandType::SLEEP), ticks_(ticks) {}

int SleepCommand::execute(SymbolTable&) {
    // The CPU worker sees the pending sleep after this instruction returns,
    // takes the process off the core and hands it to the scheduler's sleep list.
    if (ProcessControlBlock* parent = getParentProcess()) {
        parent->setSleepTicks(ticks_);
    }
    return 1;
}
