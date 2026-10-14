#pragma once

#include "Component.h"
#include <functional>
#include <string>
#include <vector>

// Result of running one command through the command interpreter.
// Kept separate from ConsoleScreen so the console has no idea what
// a "command" actually does -- it just displays whatever it's given
// and tells the caller whether an exit was requested.
struct CommandResult {
    std::vector<std::string> output;
    bool exitRequested = false;
};

// ConsoleScreen is the on-screen component for the interactive shell:
// scrollback history on top, current input line (with prompt) at the
// bottom. It never reads the keyboard itself -- everything comes in
// from the outside through handleKeyPress / handleBackspace / handleEnter.
//
// Intended flow (driven by main's input loop):
//   key captured (_getch) -> console.handleKeyPress(key) -> refreshDisplay()
//   backspace captured    -> console.handleBackspace()   -> refreshDisplay()
//   enter captured        -> console.handleEnter()        -> runs the command
//                                                             via the handler,
//                                                             then refreshDisplay()
//
// refreshDisplay() rewrites this component's text_ (inherited from
// Component) so that the next Screen::update(console) call picks up
// the change -- ConsoleScreen never draws to stdout itself.
class ConsoleScreen : public Component {
    public:
        using CommandHandler = std::function<CommandResult(const std::string& commandText)>;

        ConsoleScreen(int row, int col, int width, int height, const std::string& prompt = "Command> ");

        // --- Input events (called by the main loop as keys come in) ---
        void handleKeyPress(char key);   // ordinary printable character typed
        void handleBackspace();          // backspace / delete pressed
        void handleEnter();              // enter pressed -> runs the current input as a command

        // --- Wiring to the command interpreter ---
        // The console itself doesn't know how to run a command; it just
        // hands the typed text to whatever handler is registered and
        // prints back the lines it gets in return.
        void setCommandHandler(CommandHandler handler);

        // True exactly once after an "exit" (or similar) command result
        // requested termination; reading it clears the flag.
        bool consumeExitRequested();

        // --- Programmatic output (boot banner, help text, async ticks, etc.) ---
        void print(const std::string& text);               // splits on '\n', appends to history
        void printLines(const std::vector<std::string>& lines);
        void clearHistory();                                // wipes scrollback (e.g. "clear" command)

        // --- Accessors ---
        const std::string& getCurrentInput() const;
        const std::string& getPrompt() const;
        void setPrompt(const std::string& prompt);

    private:
        void refreshDisplay();

        std::string prompt_;
        std::string inputBuffer_;
        std::vector<std::string> history_;
        CommandHandler commandHandler_;
        bool exitRequested_;
};
