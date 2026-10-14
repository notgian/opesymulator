#include "ConsoleScreen.h"
#include <cctype>
#include <sstream>

ConsoleScreen::ConsoleScreen(int row, int col, int width, int height, const std::string& prompt)
    : Component(row, col, width, height),
      prompt_(prompt),
      inputBuffer_(""),
      commandHandler_(nullptr),
      exitRequested_(false) {
    refreshDisplay();
}

// --- Input events ---

void ConsoleScreen::handleKeyPress(char key) {
    // Guard against control characters sneaking in through this path;
    // Enter/Backspace/etc. have their own dedicated methods.
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
    // Echo what was typed into the scrollback first, so it stays visible
    // once the input line is cleared.
    history_.push_back(prompt_ + inputBuffer_);

    std::string commandText = inputBuffer_;
    inputBuffer_.clear();

    if (commandHandler_) {
        CommandResult result = commandHandler_(commandText);
        for (const std::string& line : result.output) {
            history_.push_back(line);
        }
        if (result.exitRequested) {
            exitRequested_ = true;
        }
    }

    refreshDisplay();
}

// --- Wiring ---

void ConsoleScreen::setCommandHandler(CommandHandler handler) {
    commandHandler_ = handler;
}

bool ConsoleScreen::consumeExitRequested() {
    bool requested = exitRequested_;
    exitRequested_ = false;
    return requested;
}

// --- Programmatic output ---

void ConsoleScreen::print(const std::string& text) {
    std::stringstream ss(text);
    std::string line;
    bool any = false;

    while (std::getline(ss, line)) {
        history_.push_back(line);
        any = true;
    }
    // Preserve intentional blank lines, e.g. print("")
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

// --- Accessors ---

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

// --- Internal ---

// Rebuilds this component's text_ (inherited from Component) from the
// scrollback + current input line, keeping only as many history lines
// as fit above the input line within height_. This is the one place
// that turns "console state" into "what Screen should draw".
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
