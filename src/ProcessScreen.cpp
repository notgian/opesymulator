#include "ProcessScreen.h"
#include <cctype>
#include <sstream>
#include <algorithm>

ProcessScreen::ProcessScreen(int pid, const std::string& name, int row, int col, int width, int height, const std::string& prompt)
    : ProcessControlBlock(pid, name),
      ConsoleScreen(row, col, width, height, prompt),
      onExitCallback_(nullptr),
      interpreter_(false) {

    interpreter_.registerCommand("exit", [this](const std::string&) {
        if (onExitCallback_) {
            onExitCallback_();
        }
        return CommandResult{
            CommandStatus::EXIT_REQUESTED,
            {}
        };
    }, "  exit      - exits the process screen");

    refreshDisplay();
}

void ProcessScreen::handleKeyPress(char key) {
    if (!std::isprint(static_cast<unsigned char>(key))) return;
    std::lock_guard<std::recursive_mutex> lock(screenMutex_); // Protect access
    inputBuffer_.push_back(key);
    refreshDisplay(); 
}

void ProcessScreen::handleBackspace() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_); // Protect access
    if (!inputBuffer_.empty()) {
        inputBuffer_.pop_back();
        refreshDisplay(); 
    }
}

void ProcessScreen::handleEnter() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_); // Protect access
    // Echo typed input into scrollback history
    history_.push_back(prompt_ + inputBuffer_);

    std::string commandText = inputBuffer_;
    inputBuffer_.clear();

    CommandResult result = interpreter_.executeCommand(commandText);

    if (!result.output.empty()) {
        printLines(result.output);
    }

    if (onCommandSubmitted_) {
        onCommandSubmitted_(commandText);
    }

    refreshDisplay();
}

void ProcessScreen::setOnExit(OnExitCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    onExitCallback_ = callback;
}

void ProcessScreen::refreshDisplay() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_); // Protect access
    std::vector<std::string> visible;

    int rowsForHistory = std::max(0, height_ - 1);
    int totalHistory = static_cast<int>(history_.size());
    int startIndex = std::max(0, totalHistory - rowsForHistory);

    for (int i = startIndex; i < totalHistory; ++i) {
        visible.push_back(history_[i]);
    }

    // Always keep the active typing prompt and input buffer locked to the bottom-most line
    visible.push_back(prompt_ + inputBuffer_);

    setText(visible);
}