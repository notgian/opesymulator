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
#include <optional>
#include <unordered_set>


class Kernel {
public:
    Kernel(int screenWidth, int screenHeight);
    virtual ~Kernel() = default;

    Config getConfig();

    // Process helpers shared by "screen -s" and the startup test processes
    std::shared_ptr<ProcessScreen> createProcess(const std::string& name);
    void registerProcess(const std::shared_ptr<ProcessScreen>& process);
    bool processNameTaken(const std::string& name);   // O(1), unlike findProcess()
    void generateInstructions(ProcessScreen& process, 
                              std::optional<int> minIns = std::nullopt, 
                              std::optional<int> maxIns = std::nullopt);
    std::shared_ptr<ProcessScreen> findProcess(const std::string& name);

    void scheduleProcess(std::shared_ptr<ProcessControlBlock> pcb);

    void spawnDummyProcess();

    void run();

private:
    void printWelcome();
    void clearScreen();


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
    std::unordered_set<std::string> processNames_;   // guarded by processesMutex_
    std::atomic<int> nextPid_{1};

    Config emulatorConfig_;
    void onInitialization();
    
    int screenWidth_;
    int screenHeight_;
    bool running_;
};
