#include "ConsoleScreen.h"
#include <cctype>
#include <sstream>
#include <algorithm>

ConsoleScreen::ConsoleScreen(int row, int col, int width, int height, const std::string& prompt)
    : Component(row, col, width, height),
      prompt_(prompt),
      inputBuffer_(""),
      onCommandSubmitted_(nullptr) {
    refreshDisplay();
}

void ConsoleScreen::handleKeyPress(char key) {
    if (!std::isprint(static_cast<unsigned char>(key))) return;
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    scrollOffset_ = 0;   // typing jumps back to the bottom
    inputBuffer_.push_back(key);
    refreshDisplay();
}

void ConsoleScreen::handleBackspace() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    scrollOffset_ = 0;
    if (!inputBuffer_.empty()) {
        inputBuffer_.pop_back();
    }
    refreshDisplay();
}

void ConsoleScreen::handleEnter() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    scrollOffset_ = 0;
    // Echo typed input into scrollback history
    history_.push_back(prompt_ + inputBuffer_);
    const size_t echoLine = history_.size() - 1;

    std::string commandText = inputBuffer_;
    commandHistory_.push_back(inputBuffer_);
    viewingCommandHistory_ = 0;
    inputBuffer_.clear();

    if (onCommandSubmitted_) {
        onCommandSubmitted_(commandText);
    }

    showOutputFrom(echoLine);
    refreshDisplay();
}

void ConsoleScreen::handleSpecial(int keyCode) {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    const int rows = std::max(0, height_ - 1);                        // history rows on screen
    const size_t total = history_.size();
    const size_t maxOffset = total > static_cast<size_t>(rows) ? total - rows : 0;
    const size_t page = static_cast<size_t>(std::max(1, rows - 1));   // keep one line of overlap

    switch (keyCode) {
        case PAGE_UP_KEY:
            scrollOffset_ = std::min(maxOffset, scrollOffset_ + page);
            refreshDisplay();
            break;
        case PAGE_DOWN_KEY:
            scrollOffset_ = scrollOffset_ > page ? scrollOffset_ - page : 0;
            refreshDisplay();
            break;
        case HOME_KEY:
            scrollOffset_ = maxOffset;
            refreshDisplay();
            break;
        case END_KEY:
            scrollOffset_ = 0;
            refreshDisplay();
            break;
        case UP_KEY:
            scrollOffset_ = 0;
            if (!commandHistory_.empty() && viewingCommandHistory_ < commandHistory_.size()) {
                ++viewingCommandHistory_;
                size_t targetIndex = commandHistory_.size() - viewingCommandHistory_;
                if (targetIndex < commandHistory_.size()) {
                    inputBuffer_ = commandHistory_.at(targetIndex);
                    refreshDisplay();
                }
            }
            break;
        case DOWN_KEY:
            scrollOffset_ = 0;
            if (viewingCommandHistory_ > 0) {
                --viewingCommandHistory_;
                if (viewingCommandHistory_ == 0) {
                    inputBuffer_.clear();
                } else {
                    size_t targetIndex = commandHistory_.size() - viewingCommandHistory_;
                    if (targetIndex < commandHistory_.size()) {
                        inputBuffer_ = commandHistory_.at(targetIndex);
                    }
                }
                refreshDisplay(); // Required to update the screen
            }
            break;
    }
}

void ConsoleScreen::setOnCommandSubmitted(OnCommandSubmitted callback) {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    onCommandSubmitted_ = callback;
}

void ConsoleScreen::print(const std::string& text) {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
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
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    for (const std::string& line : lines) {
        history_.push_back(line);
    }
    refreshDisplay();
}

void ConsoleScreen::clearHistory() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    history_.clear();
    refreshDisplay();
}

const std::string& ConsoleScreen::getCurrentInput() const {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    return inputBuffer_;
}

const std::string& ConsoleScreen::getPrompt() const {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    return prompt_;
}

void ConsoleScreen::setPrompt(const std::string& prompt) {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    prompt_ = prompt;
    refreshDisplay();
}

void ConsoleScreen::showOutputFrom(size_t firstLine) {
    const size_t rows = static_cast<size_t>(std::max(0, height_ - 1));
    const size_t total = history_.size();
    if (firstLine >= total || total - firstLine <= rows) return;   // fits on screen: stay at bottom
    scrollOffset_ = total - rows - firstLine;                      // first line lands at the top row
}

std::vector<std::string> ConsoleScreen::buildVisibleLines() {
    std::vector<std::string> visible;

    const int rowsForHistory = std::max(0, height_ - 1);
    const int totalHistory = static_cast<int>(history_.size());
    const int maxOffset = std::max(0, totalHistory - rowsForHistory);
    if (scrollOffset_ > static_cast<size_t>(maxOffset)) scrollOffset_ = maxOffset;   // history was cleared

    const int startIndex = std::max(0, totalHistory - rowsForHistory - static_cast<int>(scrollOffset_));
    const int endIndex = std::min(totalHistory, startIndex + rowsForHistory);

    for (int i = startIndex; i < endIndex; ++i) {
        visible.push_back(history_[i]);
    }

    if (scrollOffset_ > 0) {
        // Scrolled up: show where we are instead of the prompt
        visible.push_back("-- scrolled up " + std::to_string(scrollOffset_) +
                          " lines | PgUp/PgDn  Home/End | type to return --");
    } else {
        visible.push_back(prompt_ + inputBuffer_);
    }
    return visible;
}

void ConsoleScreen::refreshDisplay() {
    std::lock_guard<std::recursive_mutex> lock(screenMutex_);
    setText(buildVisibleLines());
}
