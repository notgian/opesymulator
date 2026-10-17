#include "PrintCommand.h"
#include "Config.h"
#include "TimeUtil.h"
#include <fstream>

PrintCommand::PrintCommand(int pid, const std::string& processName, const std::string& message)
    : ICommand(pid, ICommandType::PRINT), processName_(processName), message_(message) {}

std::string PrintCommand::logPath(const std::string& processName) {
    return "logs/" + processName + ".txt";
}

void PrintCommand::initLogFile(const std::string& processName) {
#if ENABLE_PRINT_LOGS
    std::ofstream out(logPath(processName), std::ios::trunc);
    if (out.is_open()) {
        out << "Process name: " << processName << "\n";
        out << "Logs:\n\n";
    }
#else
    (void)processName;
#endif
}

void PrintCommand::execute(SymbolTable& symbolTable) {
#if ENABLE_PRINT_LOGS
    // The executing core id is injected into the process' symbol table by the PCB
    int coreId = symbolTable.getVariable("CORE_ID");

    // Format: (08/06/2024 09:15:22AM) Core:0 "Hello world from screen_01!"
    std::ofstream outFile(logPath(processName_), std::ios::app);
    if (outFile.is_open()) {
        outFile << "(" << currentTimestamp() << ") Core:" << coreId
                << " \"" << message_ << "\"\n";
    }
#else
    (void)symbolTable;
#endif
}
