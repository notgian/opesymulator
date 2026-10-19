#pragma once

#include "ProcessControlBlock.h"
#include "ConsoleScreen.h"
#include "CommandInterpreter.h"
#include <vector>
#include <string>
#include <functional>

class ProcessScreen : public ProcessControlBlock, public ConsoleScreen {
public:
    using OnExitCallback = std::function<void()>;

    ProcessScreen(int pid, const std::string& name, int row, int col, int width, int height, const std::string& prompt = "cmd> ");

    // Input handling functions
    void handleKeyPress(char key);
    void handleBackspace();
    void handleEnter();

    void setOnExit(OnExitCallback callback);

private:
    void refreshDisplay();

    CommandInterpreter interpreter_;
    OnExitCallback onExitCallback_;
};