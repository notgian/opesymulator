#include "PrintCommand.h"
#include "TimeUtil.h"
#include "ProcessScreen.h"
#include <fstream>

PrintCommand::PrintCommand(int pid, const std::string& message)
    : ICommand(pid, ICommandType::PRINT), message_(message) {}


void PrintCommand::execute(SymbolTable& symbolTable) {
    // The executing core id is injected into the process' symbol table by the PCB
    int coreId = symbolTable.getVariable("CORE_ID");

    std::string formattedMessage = "[" + currentTimestamp() + "]: " +
                                   " \"" + message_ + "\"";

    // Cast the parent process to ProcessScreen to print directly to its screen buffer
    if (ProcessScreen* screen = dynamic_cast<ProcessScreen*>(getParentProcess())) {
        screen->print(formattedMessage);
    }
}
