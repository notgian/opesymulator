#pragma once

#include "Component.h"
#include <string>
#include <vector>
#include <functional>
#include <mutex>

constexpr int UP_KEY = 72;
constexpr int DOWN_KEY = 80;

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

    std::string prompt_;
    std::string inputBuffer_;
    std::vector<std::string> history_;
    OnCommandSubmitted onCommandSubmitted_;
    mutable std::recursive_mutex screenMutex_;

    std::vector<std::string> commandHistory_;
    size_t viewingCommandHistory_ = 0;
};
