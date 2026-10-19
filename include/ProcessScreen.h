#pragma once

#include "ProcessControlBlock.h"
#include "ConsoleScreen.h"
#include <vector>
#include <string>
#include <functional>

class ProcessScreen : public ProcessControlBlock, public ConsoleScreen {
public:
    using OnCommandSubmitted = std::function<void(const std::string&)>;

    ProcessScreen(int pid, const std::string& name, int row, int col, int width, int height, const std::string& prompt = "cmd> ");

    // Input handling functions
    void handleKeyPress(char key);
    void handleBackspace();
    void handleEnter();
    void setOnCommandSubmitted(OnCommandSubmitted callback);

    // Screen text output functions (updates history and refreshes display)
    void print(const std::string& text);
    void printLines(const std::vector<std::string>& lines);
    void clearHistory();

    // Getters / Setters
    const std::string& getCurrentInput() const;
    const std::string& getPrompt() const;
    void setPrompt(const std::string& prompt);
    const std::vector<std::string>& getHistory() const;

private:
    void refreshDisplay();

    std::string prompt_;
    std::string inputBuffer_;
    std::vector<std::string> history_; // Holds the scrollback contents of the screen
    OnCommandSubmitted onCommandSubmitted_;
};