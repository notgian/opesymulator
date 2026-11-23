#include "PrintCommand.h"
#include "TimeUtil.h"
#include "ProcessScreen.h"

PrintCommand::PrintCommand(int pid, const std::string& message)
    : ICommand(pid, ICommandType::PRINT), message_(message) {}

PrintCommand::PrintCommand(int pid, const std::string& message, const std::string& varName)
    : ICommand(pid, ICommandType::PRINT), message_(message), varName_(varName) {}

int PrintCommand::execute(SymbolTable& symbolTable) {
    ProcessControlBlock* parent = getParentProcess();
    int coreId = parent ? parent->getCurrentCoreId() : 0;

    std::string text = message_;
    if (!varName_.empty()) {
        // Undeclared variables read as 0; ensures symbol table page is resident
        text += std::to_string(symbolTable.getVariable(varName_));
    }

    std::string formattedMessage = "(" + currentTimestamp() + "): " + "Core: " + std::to_string(coreId) +
                                   " \"" + text + "\"";

    if (ProcessScreen* screen = dynamic_cast<ProcessScreen*>(parent)) {
        screen->appendLogs(formattedMessage);
    }

    return 1;
}
