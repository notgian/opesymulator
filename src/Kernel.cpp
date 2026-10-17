#include "Kernel.h"
#include "PrintCommand.h"  // TODO: temporary for now
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

    // ---------------------------------------------------------------------
    // --- TEST CASE SETUP: 10 processes x 100 print commands at startup ---
    // ---------------------------------------------------------------------
    // TODO: remove for actual emulator

#if ENABLE_PRINT_LOGS
    CreateDirectoryA("logs", NULL);   // logs/screen_01.txt ... logs/screen_10.txt
#endif

    std::vector<std::shared_ptr<ProcessControlBlock>> allProcesses;

    for (int i = 0; i < NUM_TEST_PROCESSES; ++i) {
        int pid = i + 1;
        char nameBuf[32];
        std::snprintf(nameBuf, sizeof(nameBuf), "screen_%02d", pid);
        std::string name = nameBuf;

        auto pcb = std::make_shared<ProcessControlBlock>(pid, name);

        // Each process owns a log file: "Process name: screen_01 / Logs:"
        PrintCommand::initLogFile(name);

        for (int j = 0; j < PRINTS_PER_PROCESS; ++j) {
            pcb->addCommand(std::make_shared<PrintCommand>(pid, name, "Hello world from " + name + "!"));
        }

        // Keep track of all processes for the screen -ls command lookup
        allProcesses.push_back(pcb);
    }

    // --- TEMPORARY COMMAND REGISTRATION ---
    // 'screen -ls' prints the running / finished process lists
    interpreter_.registerCommand("screen", [this, &allProcesses](const std::string& args) {
        // trim whitespace around the argument
        size_t b = args.find_first_not_of(' ');
        size_t e = args.find_last_not_of(' ');
        std::string arg = (b == std::string::npos) ? "" : args.substr(b, e - b + 1);

        if (arg != "-ls") {
            return CommandResult{CommandStatus::CMD_ERROR, {"Usage: screen -ls"}};
        }

        const std::string separator(38, '-');
        char buf[160];
        std::vector<std::string> out;

        out.push_back(separator);

        // ---- Running processes (one per busy core) ----
        out.push_back("Running processes:");
        std::vector<RunningEntry> running = cpu_.getRunningProcesses();
        for (const auto& entry : running) {
            const auto& p = entry.process;
            std::snprintf(buf, sizeof(buf), "%-12s (%s)   Core: %d   %d / %d",
                          p->getName().c_str(), p->getCreationTime().c_str(), entry.coreId,
                          p->getProgramCounter(), p->getTotalCommands());
            out.push_back(buf);
        }

        out.push_back("");

        // ---- Finished processes ----
        out.push_back("Finished processes:");
        for (const auto& p : allProcesses) {
            if (p->getState() != ProcessState::TERMINATED) continue;
            std::snprintf(buf, sizeof(buf), "%-12s (%s)   Finished   %d / %d",
                          p->getName().c_str(), p->getCreationTime().c_str(),
                          p->getProgramCounter(), p->getTotalCommands());
            out.push_back(buf);
        }

        out.push_back(separator);

        return CommandResult{CommandStatus::SUCCESS, out};
    });

    // Multi-threaded execution: 1 scheduler thread + 1 worker thread per core.
    // Everything is queued now (first-come-first-serve order = screen_01 first).
    for (const auto& pcb : allProcesses) {
        scheduler_.addProcess(pcb);
    }
    cpu_.start();
    scheduler_.start(cpu_);
    // ---------------------------------------------------------------------

    // Main (UI) thread: keyboard input + rendering only
    while (running_) {
        if (_kbhit()) {
            int intercepted = _getch();

            switch (intercepted) {
                case '\r': // Enter
                    consoleScreen_.handleEnter();
                    break;
                case '\b': // Backspace
                case 127:  // DEL
                    consoleScreen_.handleBackspace();
                    break;
                default:
                    if (intercepted >= 32 && intercepted < 127) {
                        consoleScreen_.handleKeyPress(static_cast<char>(intercepted));
                    }
                    break;
            }
        }

        // Update display buffer and render
        mainDisplay_.update(consoleScreen_);
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