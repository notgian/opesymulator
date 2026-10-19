#include "ProcessScreen.h"
#include <cctype>
#include <sstream>
#include <algorithm>

ProcessScreen::ProcessScreen(int pid, const std::string& name, int row, int col, int width, int height, const std::string& prompt)
    : ProcessControlBlock(pid, name),
      ConsoleScreen(row, col, width, height, prompt) {
    refreshDisplay();
}

void ProcessScreen::handleKeyPress(char key) {
    if (!std::isprint(static_cast<unsigned char>(key))) return;
    inputBuffer_.push_back(key); // Uses ConsoleScreen's inputBuffer_
    refreshDisplay(); 
}

void ProcessScreen::handleBackspace() {
    if (!inputBuffer_.empty()) {
        inputBuffer_.pop_back(); // Uses ConsoleScreen's inputBuffer_
        refreshDisplay(); 
    }
}

void ProcessScreen::handleEnter() {
    // Echo typed input into scrollback history
    history_.push_back(prompt_ + inputBuffer_); // Uses ConsoleScreen's history_ and prompt_

    std::string commandText = inputBuffer_;
    inputBuffer_.clear();

    if (onCommandSubmitted_) {
        onCommandSubmitted_(commandText);
    }

    refreshDisplay();
}

void ProcessScreen::setOnCommandSubmitted(OnCommandSubmitted callback) {
    onCommandSubmitted_ = callback; // Uses ConsoleScreen's callback member
}

void ProcessScreen::print(const std::string& text) {
    std::stringstream ss(text);
    std::string line;
    bool any = false;

    while (std::getline(ss, line)) {
        history_.push_back(line); // Appends to ConsoleScreen's shared history vector
        any = true;
    }
    if (!any) {
        history_.push_back("");
    }

    refreshDisplay(); 
}

void ProcessScreen::printLines(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        history_.push_back(line); // Appends to ConsoleScreen's shared history vector
    }
    refreshDisplay(); 
}

void ProcessScreen::clearHistory() {
    history_.clear();
    refreshDisplay();
}

const std::string& ProcessScreen::getCurrentInput() const {
    return inputBuffer_;
}

const std::string& ProcessScreen::getPrompt() const {
    return prompt_;
}

void ProcessScreen::setPrompt(const std::string& prompt) {
    prompt_ = prompt;
    refreshDisplay();
}

const std::vector<std::string>& ProcessScreen::getHistory() const {
    return history_;
}

void ProcessScreen::refreshDisplay() {
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