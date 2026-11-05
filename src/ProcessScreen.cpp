#include "ProcessScreen.h"
#include <cctype>
#include <sstream>
#include <algorithm>

ProcessScreen::ProcessScreen(int pid, const std::string& name, int row, int col, int width, int height, const std::string& prompt)
    : ProcessControlBlock(pid, name),
      ConsoleScreen(row, col, width, height, prompt),
      onExitCallback_(nullptr),
      interpreter_(false) {

    interpreter_.registerCommand("process-smi", [this](const std::string&) {
        printLogs();

        return CommandResult{
            CommandStatus::SUCCESS,
            {}
        };
    }, "  process-smi      - prints process logs and info");

    interpreter_.registerCommand("exit", [this](const std::string&) {
        if (onExitCallback_) {
            onExitCallback_();
        }
        return CommandResult{
            CommandStatus::EXIT_REQUESTED,
            {}
        };
    }, "  exit             - exits the process screen");

    refreshDisplay();
}

void ProcessScreen::appendLogs(std::string& printContent) {
    logs_.push_back(printContent);
}
    
void ProcessScreen::printLogs() {
    print("\nProcess name: " + getName());
    print("ID: " + std::to_string(getPID()));
    print("Instructions: " + std::to_string(getProgramCounter()) + "/" + std::to_string(getTotalCommands()));
    print("Logs:");
    printLines(logs_);

    if (getState() == ProcessState::TERMINATED) {
        print("\nFinished!");
    }
}

void ProcessScreen::handleKeyPress(char key) {
    if (!std::isprint(static_cast<unsigned char>(key))) return;
    std::lock_guard<std::recursive_mutex> lock(screenMutex_); // Protect access
    scrollOffset_ = 0;   // typing jumps back to the bottom
    inputBuffer_.push_back(key);
    refreshDisplay(); 
}

void ProcessScreen::handleBackspace() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_); // Protect access
    scrollOffset_ = 0;
    if (!inputBuffer_.empty()) {
        inputBuffer_.pop_back();
    }
    refreshDisplay();
}

void ProcessScreen::handleEnter() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_); // Protect access
    scrollOffset_ = 0;
    // Echo typed input into scrollback history
    history_.push_back(prompt_ + inputBuffer_);
    const size_t echoLine = history_.size() - 1;

    std::string commandText = inputBuffer_;
    inputBuffer_.clear();

    CommandResult result = interpreter_.executeCommand(commandText);

    if (!result.output.empty()) {
        printLines(result.output);
    }

    if (onCommandSubmitted_) {
        onCommandSubmitted_(commandText);
    }

    showOutputFrom(echoLine);
    refreshDisplay();
}

void ProcessScreen::setOnExit(OnExitCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    onExitCallback_ = callback;
}

void ProcessScreen::refreshDisplay() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_); // Protect access
    // Same windowing as the main console: honors scrolling, otherwise keeps the typing
    // prompt locked to the bottom-most line
    setText(buildVisibleLines());
}