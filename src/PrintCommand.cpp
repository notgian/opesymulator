#include "PrintCommand.h"
#include "TimeUtil.h"
#include "ProcessScreen.h"
#include <fstream>

PrintCommand::PrintCommand(int pid, const std::string& message)
    : ICommand(pid, ICommandType::PRINT), message_(message) {}

PrintCommand::PrintCommand(int pid, const std::string& message, const std::string& varName)
    : ICommand(pid, ICommandType::PRINT), message_(message), varName_(varName) {}

int PrintCommand::execute(SymbolTable& symbolTable) {
    // The executing core id is injected into the process' symbol table by the PCB
    int coreId = symbolTable.getVariable("CORE_ID");

    std::string text = message_;
    if (!varName_.empty()) {
        // Undeclared variables read as 0 (same rule as ADD / SUBTRACT)
        text += std::to_string(symbolTable.getVariable(varName_));
    }

    std::string formattedMessage = "(" + currentTimestamp() + "): " + "Core: " + std::to_string(coreId) +
                                   " \"" + text + "\"";

    // Cast the parent process to ProcessScreen to print directly to its screen buffer
    if (ProcessScreen* screen = dynamic_cast<ProcessScreen*>(getParentProcess())) {
        screen->appendLogs(formattedMessage);
        // screen->print(formattedMessage);
    }

    return 1;
}