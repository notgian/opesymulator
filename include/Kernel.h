#pragma once

#include "Display.h"
#include "ConsoleScreen.h"
#include "CommandInterpreter.h"
#include "CPU.h"
#include "Scheduler.h"
#include "Config.h"
#include "ProcessScreen.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>


class Kernel {
public:
    Kernel(int screenWidth, int screenHeight);
    virtual ~Kernel() = default;

    void run();

private:
    void printWelcome();
    void clearScreen();

    // Process helpers shared by "screen -s" and the startup test processes
    std::shared_ptr<ProcessScreen> createProcess(const std::string& name);
    void registerProcess(const std::shared_ptr<ProcessScreen>& process);
    void generateInstructions(ProcessScreen& process);
    std::shared_ptr<ProcessScreen> findProcess(const std::string& name);
    void attachScreen(const std::shared_ptr<ProcessScreen>& process);

    // Shared by "screen -ls" and "report-util"
    std::vector<std::string> buildUtilizationReport();

    Display mainDisplay_;
    ConsoleScreen consoleScreen_;
    std::shared_ptr<ConsoleScreen> activeScreen_;
    CommandInterpreter interpreter_;

    std::unique_ptr<CPU> cpu_;
    std::unique_ptr<Scheduler> scheduler_;

    std::vector<std::shared_ptr<ProcessControlBlock>> allProcesses_;
    std::mutex processesMutex_;
    std::atomic<int> nextPid_{1};

    Config emulatorConfig_;
    void onInitialization();
    
    int screenWidth_;
    int screenHeight_;
    bool running_;
};
