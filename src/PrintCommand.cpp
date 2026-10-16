#include "PrintCommand.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

PrintCommand::PrintCommand(int pid, const std::string& message)
    : ICommand(pid, ICommandType::PRINT), message_(message) {}

void PrintCommand::execute(SymbolTable& symbolTable) {
    // Fetch the executing core ID from the process's symbol table
    int coreId = symbolTable.getVariable("CORE_ID"); 

    // Get current system timestamp
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    std::string timestamp = ss.str();

    // Define unique filename per process based on PID
    std::string filename = "process_" + std::to_string(pid_) + ".txt";

    // Open file in append mode and write log entry
    std::ofstream outFile(filename, std::ios::app);
    if (outFile.is_open()) {
        outFile << "[" << timestamp << "] Core: " << coreId << " - " << message_ << "\n";
        outFile.close();
    }
}