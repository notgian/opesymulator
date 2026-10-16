#include "Kernel.h"
#include "PrintCommand.h"  // TODO: temporary for now
#include <iostream>
#include <conio.h>
#include <windows.h>

Kernel::Kernel(int screenWidth, int screenHeight)
    : mainDisplay_(screenWidth, screenHeight),
      consoleScreen_(0, 0, screenWidth, screenHeight, "Command> "),
      cpu_(4),
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
    // --- SIMULATION SETUP: Pre-initialize 10 processes before the loop ---
    // ---------------------------------------------------------------------
    // TODO: remove for actual emulator
    struct ProcessArrival {
        long long arrivalTick;
        std::shared_ptr<ProcessControlBlock> pcb;
    };

    std::vector<ProcessArrival> pendingProcesses;
    for (int i = 0; i < 10; ++i) {
        int pid = i + 1;
        auto pcb = std::make_shared<ProcessControlBlock>(pid, "Process_" + std::to_string(pid));
        
        // Add 100 print commands to each process
        for (int j = 0; j < 100; ++j) {
            std::string logMsg = "Execution log entry #" + std::to_string(j + 1) + " for PID " + std::to_string(pid);
            pcb->addCommand(std::make_shared<PrintCommand>(pid, logMsg));
        }

        // Stagger arrival times (e.g., a new process arrives every 30 kernel ticks)
        long long arrivalTick = static_cast<long long>(i) * 30;
        pendingProcesses.push_back({arrivalTick, pcb});
    }

    // Temporary simulation tracking variable
    long long kernelTickCounter = 0;
    // ---------------------------------------------------------------------

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

        // ----------------------------------
        // --- PROCESS ARRIVAL SIMULATION ---
        // ----------------------------------
        // Check if any pending processes have reached their arrival time
        // TODO: remove for actual emulator
        for (auto it = pendingProcesses.begin(); it != pendingProcesses.end(); ) {
            if (kernelTickCounter >= it->arrivalTick) {
                // Add the process to the scheduler's ready pool
                scheduler_.addProcess(it->pcb);
                // Remove from pending list so it only arrives once
                it = pendingProcesses.erase(it);
            } else {
                ++it;
            }
        }
        // ----------------------------------

        // cpu tick and scheduler
        cpu_.executeCycle();
        scheduler_.update(cpu_);
        // TODO: scheduler does its thing here

        // Update display buffer and render
        mainDisplay_.update(consoleScreen_);
        mainDisplay_.draw();
        mainDisplay_.clearBuffer();

        // Increment simulation tick counter
        // TODO: remove for actual emulator
        kernelTickCounter++;

        // Brief pause to prevent 100% CPU usage
        Sleep(10); 
    }
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