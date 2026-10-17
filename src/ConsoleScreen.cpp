#include "ConsoleScreen.h"
#include <cctype>
#include <sstream>

ConsoleScreen::ConsoleScreen(int row, int col, int width, int height, const std::string& prompt)
    : Component(row, col, width, height),
      prompt_(prompt),
      inputBuffer_(""),
      onCommandSubmitted_(nullptr) {
    refreshDisplay();
}

void ConsoleScreen::handleKeyPress(char key) {
    if (!std::isprint(static_cast<unsigned char>(key))) return;
    inputBuffer_.push_back(key);
    refreshDisplay();
}

void ConsoleScreen::handleBackspace() {
    if (!inputBuffer_.empty()) {
        inputBuffer_.pop_back();
        refreshDisplay();
    }
}

void ConsoleScreen::handleEnter() {
    // Echo typed input into scrollback history
    history_.push_back(prompt_ + inputBuffer_);

    std::string commandText = inputBuffer_;
    inputBuffer_.clear();

    if (onCommandSubmitted_) {
        onCommandSubmitted_(commandText);
    }

    refreshDisplay();
}

void ConsoleScreen::setOnCommandSubmitted(OnCommandSubmitted callback) {
    onCommandSubmitted_ = callback;
}

void ConsoleScreen::print(const std::string& text) {
    std::stringstream ss(text);
    std::string line;
    bool any = false;

    while (std::getline(ss, line)) {
        history_.push_back(line);
        any = true;
    }
    if (!any) history_.push_back("");

    refreshDisplay();
}

void ConsoleScreen::printLines(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        history_.push_back(line);
    }
    refreshDisplay();
}

void ConsoleScreen::clearHistory() {
    history_.clear();
    refreshDisplay();
}

const std::string& ConsoleScreen::getCurrentInput() const {
    return inputBuffer_;
}

const std::string& ConsoleScreen::getPrompt() const {
    return prompt_;
}

void ConsoleScreen::setPrompt(const std::string& prompt) {
    prompt_ = prompt;
    refreshDisplay();
}

void ConsoleScreen::refreshDisplay() {
    std::vector<std::string> visible;

    int rowsForHistory = std::max(0, height_ - 1);
    int totalHistory = static_cast<int>(history_.size());
    int startIndex = std::max(0, totalHistory - rowsForHistory);

    for (int i = startIndex; i < totalHistory; ++i) {
        visible.push_back(history_[i]);
    }

    visible.push_back(prompt_ + inputBuffer_);

    setText(visible);
}