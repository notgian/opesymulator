#pragma once

#include "Component.h"
#include <string>
#include <vector>
#include <functional>
#include <mutex>

constexpr int UP_KEY = 72;
constexpr int DOWN_KEY = 80;
constexpr int HOME_KEY = 71;
constexpr int END_KEY = 79;
constexpr int PAGE_UP_KEY = 73;
constexpr int PAGE_DOWN_KEY = 81;

class ConsoleScreen : public Component {
public:
    using OnCommandSubmitted = std::function<void(const std::string& commandText)>;

    ConsoleScreen(int row, int col, int width, int height, const std::string& prompt = "Command> ");

    // --- Input Events ---
    virtual void handleKeyPress(char key);
    virtual void handleBackspace();
    virtual void handleEnter();
    virtual void handleSpecial(int keycode);

    // --- Wiring ---
    void setOnCommandSubmitted(OnCommandSubmitted callback);

    // ---  Output & Display Control ---
    void print(const std::string& text);
    void printLines(const std::vector<std::string>& lines);
    void clearHistory();

    // --- Accessors ---
    const std::string& getCurrentInput() const;
    const std::string& getPrompt() const;
    void setPrompt(const std::string& prompt);

protected:
    virtual void refreshDisplay();

    // Builds the rows to show (caller must hold screenMutex_): the history window picked by
    // scrollOffset_, then either the prompt line or, while scrolled up, a short hint line.
    std::vector<std::string> buildVisibleLines();

    // If the output printed since history line `firstLine` is longer than the screen, scroll so
    // that line sits at the top (like `less`) instead of showing only the tail of the output.
    // Caller must hold screenMutex_.
    void showOutputFrom(size_t firstLine);

    std::string prompt_;
    std::string inputBuffer_;
    std::vector<std::string> history_;
    OnCommandSubmitted onCommandSubmitted_;
    mutable std::recursive_mutex screenMutex_;

    std::vector<std::string> commandHistory_;
    size_t viewingCommandHistory_ = 0;

    // How many lines the view is scrolled up from the bottom (0 = following the newest output)
    size_t scrollOffset_ = 0;
};
