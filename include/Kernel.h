#pragma once

#include "Display.h"
#include "ConsoleScreen.h"
#include "CommandInterpreter.h"
#include "CPU.h"
#include "Scheduler.h"
#include "Config.h"


class Kernel {
public:
    Kernel(int screenWidth, int screenHeight);
    virtual ~Kernel() = default;

    void run();

private:
    void printWelcome();
    void clearScreen();

    Display mainDisplay_;
    ConsoleScreen consoleScreen_;
    std::shared_ptr<ConsoleScreen> activeScreen_;
    CommandInterpreter interpreter_;

    CPU cpu_;
    Scheduler scheduler_;

    std::vector<std::shared_ptr<ProcessControlBlock>> allProcesses_;

    Config emulatorConfig_;
    void onInitialization();
    
    int screenWidth_;
    int screenHeight_;
    bool running_;
};
