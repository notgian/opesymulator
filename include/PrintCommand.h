#pragma once

#include "ICommand.h"
#include "SymbolTable.h"

#include <string>

class PrintCommand : public ICommand {
public:
    PrintCommand(int pid, const std::string& processName, const std::string& message);

    void execute(SymbolTable& symbolTable) override;

    // Path of the log file of a process, e.g. logs/screen_01.txt
    static std::string logPath(const std::string& processName);

    // Creates (or truncates) the process' log file and writes its header:
    //   Process name: screen_01
    //   Logs:
    static void initLogFile(const std::string& processName);

private:
    std::string processName_;
    std::string message_;
};
