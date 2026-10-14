#include <iostream>
#include <string>
#include <conio.h>
#include <windows.h>
#include "Screen.h"
#include "Component.h"
#include "ConsoleScreen.h"
#include "CommandInterpreter.h"

void print_welcome(ConsoleScreen &console) {
    console.print("    ____  ____  _____________   ____  _____   ____    ___  __________  ____ ");
    console.print("   / __ \\/ __ \\/ ____/ ___/\\ \\/ /  |/  / / / / /   /   |/_  __/ __ \\/ __ \\");
    console.print("  / / / / /_/ / __/  \\__ \\  \\  / /|_/ / / / / /   / /| | / / / / / / /_/ /");
    console.print(" / /_/ / ____/ /___ ___/ /  / / /  / / /_/ / /___/ ___ |/ / / /_/ / _, _/ ");
    console.print(" \\____/_/   /_____//____/  /_/_/  /_/\\____/_____/_/  |_/_/  \\____/_/ |_| ");
    console.print("");
    console.print("Welcome");
    console.print("CSOPESY Emulator");
    console.print("Agsalon - Ercia - Garcia - Ortha");
    console.print("");
}

void clearScreen() {
    std::cout << "\033[2J\033[H";
}

int main(int argc, char *argv[]) {
    int screenWidth = 100;
    int screenHeight = 25;
    bool running = true;

    Screen MainScreen(screenWidth, screenHeight);
    ConsoleScreen Console(0, 0, screenWidth, screenHeight, "Command> ");
    CommandInterpreter interpreter;

    print_welcome(Console);
    Console.print("Type 'help' to see the available commands.");

    // Bridge the console input action to the interpreter and handle statuses
    Console.setOnCommandSubmitted([&Console, &interpreter, &running](const std::string& commandText) {
        CommandResult result = interpreter.executeCommand(commandText);

        // Display any text lines returned by the interpreter
        Console.printLines(result.output);

        // Act on the separated status payload
        switch (result.status) {
            case CommandStatus::EXIT_REQUESTED:
                running = false;
                break;
            case CommandStatus::CLEAR_HISTORY:
                Console.clearHistory();
                break;
            case CommandStatus::SUCCESS:
            case CommandStatus::NOT_FOUND:
            case CommandStatus::CMD_ERROR:
                break;
        }
    });

    clearScreen();
    while (running) {
        if (_kbhit()) {
            int intercepted = _getch();

            switch (intercepted) {
                case '\r': // Enter
                    Console.handleEnter();
                    break;
                case '\b': // Backspace
                case 127:  // DEL
                    Console.handleBackspace();
                    break;
                default:
                    if (intercepted >= 32 && intercepted < 127) {
                        Console.handleKeyPress(static_cast<char>(intercepted));
                    }
                    break;
            }
        }

        MainScreen.update(Console);
        MainScreen.draw();
        MainScreen.clearBuffer();

        Sleep(1);
    }

    return 0;
}