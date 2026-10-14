#pragma once

#include "Display.h"
#include "ConsoleScreen.h"
#include "CommandInterpreter.h"


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
    CommandInterpreter interpreter_;
    bool running_;
};