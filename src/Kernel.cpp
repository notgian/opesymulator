#include "Kernel.h"
#include <iostream>
#include <conio.h>
#include <windows.h>

Kernel::Kernel(int screenWidth, int screenHeight)
    : mainDisplay_(screenWidth, screenHeight),
      consoleScreen_(0, 0, screenWidth, screenHeight, "Command> "),
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

        Sleep(1); // Brief pause to prevent 100% CPU usage
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