#include "Kernel.h"
#include "PrintCommand.h"  // TODO: temporary for now
#include "ProcessScreen.h"
#include "Config.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <conio.h>
#include <windows.h>

Kernel::Kernel(int screenWidth, int screenHeight)
    : mainDisplay_(screenWidth, screenHeight),
      consoleScreen_(0, 0, screenWidth, screenHeight, "Command> "),
      cpu_(NUM_CORES),
      scheduler_(),
      screenWidth_(screenWidth),
      screenHeight_(screenHeight),
      activeScreen_(nullptr),
      running_(true) {
    
    printWelcome();
    consoleScreen_.print("Type 'help' to see the available commands.");

    // Wire up the command submission callback to the interpreter
    consoleScreen_.setOnCommandSubmitted([this](const std::string& commandText) {
        CommandResult result = interpreter_.executeCommand(commandText);

        // Print outputs back to the console screen
        consoleScreen_.printLines(result.output);

        // Handle system level command statuses
        switch (result.status) {
            case CommandStatus::EXIT_REQUESTED:
                running_ = false;
                break;
            case CommandStatus::CLEAR_HISTORY:
                consoleScreen_.clearHistory();
                break;
            case CommandStatus::SUCCESS:
            case CommandStatus::NOT_FOUND:
            case CommandStatus::CMD_ERROR:
                break;
        }
    });
}

void Kernel::run() {
    clearScreen();
    allProcesses_.clear();

    // ---------------------------------------------------------------------
    // --- TEST CASE SETUP: 10 processes x 100 print commands at startup ---
    // ---------------------------------------------------------------------


    for (int i = 0; i < NUM_TEST_PROCESSES; ++i) {
        int pid = i + 1;
        char nameBuf[32];
        std::snprintf(nameBuf, sizeof(nameBuf), "screen_%02d", pid);
        std::string name = nameBuf;

        auto pcb = std::make_shared<ProcessScreen>(pid, name, 0, 0, screenWidth_, screenHeight_);

        for (int j = 0; j < PRINTS_PER_PROCESS; ++j) {
            pcb->addCommand(std::make_shared<PrintCommand>(pid, "Hello world " + std::to_string(j) + " from " + name + "!"));
        }

        // Keep track of all processes for the screen -ls command lookup
        allProcesses_.push_back(pcb);
    }

    // --- TEMPORARY COMMAND REGISTRATION ---
    // 'screen -ls' prints the running / finished process lists
    interpreter_.registerCommand("screen -ls", [this](const std::string& args) {
        const std::string separator(38, '-');
        char buf[160];
        std::vector<std::string> out;

        out.push_back(separator);

        // ---- Running processes (one per busy core) ----
        out.push_back("Running processes:");
        std::vector<RunningEntry> running = cpu_.getRunningProcesses();
        for (const auto& entry : running) {
            const auto& p = entry.process;
            std::snprintf(buf, sizeof(buf), "%-12s (%s)    Core: %d   %d / %d",
                    p->getName().c_str(), p->getCreationTime().c_str(), entry.coreId,
                    p->getProgramCounter(), p->getTotalCommands());
            out.push_back(buf);
        }

        out.push_back("");

        // ---- Finished processes ----
        out.push_back("Finished processes:");
        for (const auto& p : allProcesses_) {
            if (p->getState() != ProcessState::TERMINATED) continue;
            std::snprintf(buf, sizeof(buf), "%-12s (%s)    Finished   %d / %d",
                    p->getName().c_str(), p->getCreationTime().c_str(),
                    p->getProgramCounter(), p->getTotalCommands());
            out.push_back(buf);
        }

        out.push_back(separator);

        return CommandResult{CommandStatus::SUCCESS, out};
    });

    interpreter_.registerCommand("screen -s", [this](const std::string& args) {
        std::string procName = args;
        
        // Trim whitespace around the process name
        size_t nb = procName.find_first_not_of(' ');
        size_t ne = procName.find_last_not_of(' ');
        if (nb != std::string::npos) {
            procName = procName.substr(nb, ne - nb + 1);
        }

        // Search for the process in allProcesses_
        for (const auto& pcb : allProcesses_) {
            if (pcb->getName() == procName) {
                // Cast and assign to activeScreen_ (using std::dynamic_pointer_cast since allProcesses_ holds base/derived pointers)
                activeScreen_ = std::dynamic_pointer_cast<ProcessScreen>(pcb);
                if (activeScreen_) {
                    return CommandResult{CommandStatus::SUCCESS, {"Switched to screen: " + procName}};
                }
            }
        }

        return CommandResult{CommandStatus::CMD_ERROR, {"Process not found: " + procName}};
    });

    interpreter_.registerCommand("screen", [this](const std::string& args) {
        return CommandResult{CommandStatus::CMD_ERROR, {"Usage: screen -ls OR screen -s <process_name>"}};
    });

    // Multi-threaded execution: 1 scheduler thread + 1 worker thread per core.
    // Everything is queued now (first-come-first-serve order = screen_01 first).
    for (const auto& pcb : allProcesses_) {
        scheduler_.addProcess(pcb);
    }

    cpu_.start();
    scheduler_.start(cpu_);
    // ---------------------------------------------------------------------

    // Main (UI) thread: keyboard input + rendering only
    while (running_) {
        ConsoleScreen* currentScreen = activeScreen_ ? static_cast<ConsoleScreen*>(activeScreen_.get()) : static_cast<ConsoleScreen*>(&consoleScreen_);

        if (_kbhit()) {
            int intercepted = _getch();

            switch (intercepted) {
                case '\r': // Enter
                    currentScreen->handleEnter();
                    break;
                case '\b': // Backspace
                case 127:  // DEL
                    currentScreen->handleBackspace();
                    break;
                default:
                    if (intercepted >= 32 && intercepted < 127) {
                        currentScreen->handleKeyPress(static_cast<char>(intercepted));
                    }
                    break;
            }
        }

        // Update display buffer and render
        mainDisplay_.update(*currentScreen);
        mainDisplay_.draw();
        mainDisplay_.clearBuffer();

        // Brief pause to prevent 100% CPU usage
        Sleep(10);
    }

    // Shut the scheduler thread and the core worker threads down cleanly
    scheduler_.stop();
    cpu_.stop();
}

void Kernel::printWelcome() {
    consoleScreen_.print("    ____  ____  _____________   ____  _____   ____    ___  __________  ____ ");
    consoleScreen_.print("   / __ \\/ __ \\/ ____/ ___/\\ \\/ /  |/  / / / / /   /   |/_  __/ __ \\/ __ \\");
    consoleScreen_.print("  / / / / /_/ / __/  \\__ \\  \\  / /|_/ / / / / /   / /| | / / / / / / /_/ /");
    consoleScreen_.print(" / /_/ / ____/ /___ ___/ /  / / /  / / /_/ / /___/ ___ |/ / / /_/ / _, _/ ");
    consoleScreen_.print(" \\____/_/   /_____//____/  /_/_/  /_/\\____/_____/_/  |_/_/  \\____/_/ |_| ");
    consoleScreen_.print("");
    consoleScreen_.print("Welcome");
    consoleScreen_.print("CSOPESY Emulator");
    consoleScreen_.print("Agsalon - Ercia - Garcia - Ortha");
    consoleScreen_.print("");
}

void Kernel::clearScreen() {
    std::cout << "\033[2J\033[H";
}
