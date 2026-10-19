#include "CommandInterpreter.h"

CommandInterpreter::CommandInterpreter() {
    // Register default built-in commands
    registerCommand("help", [](const std::string&) {
        return CommandResult{
            CommandStatus::SUCCESS,
            {
                "Available commands:",
                "  help                 - displays this list",
                "  screen -ls           - lists running and finished processes",
                "  clear                - clears the console history",
                "  exit                 - terminates the console"
            }
        };
    });

    registerCommand("clear", [](const std::string&) {
        return CommandResult{
            CommandStatus::CLEAR_HISTORY,
            {} // Output is empty because history clearing is handled via status
        };
    });

    registerCommand("exit", [](const std::string&) {
        return CommandResult{
            CommandStatus::EXIT_REQUESTED,
            {"bye bye"}
        };
    });
}

void CommandInterpreter::registerCommand(const std::string& commandName, CommandHandler handler) {
    handlers_[commandName] = handler;
}

CommandResult CommandInterpreter::executeCommand(const std::string& commandLine) {
    if (commandLine.empty()) {
        return CommandResult{CommandStatus::SUCCESS, {}};
    }

    std::string currentCommand = commandLine;

    // Try matching from the longest possible prefix (by space separation) downwards
    while (true) {
        auto it = handlers_.find(currentCommand);
        if (it != handlers_.end()) {
            // Found a match! Extract the remaining part as arguments
            std::string args = "";
            if (currentCommand.length() < commandLine.length()) {
                args = commandLine.substr(currentCommand.length());
                // Strip leading space from arguments if present
                if (!args.empty() && args[0] == ' ') {
                    args.erase(0, 1);
                }
            }
            return it->second(args);
        }

        // Find the last space to shorten the command candidate by one word
        size_t lastSpace = currentCommand.find_last_of(' ');
        if (lastSpace == std::string::npos) {
            break; // No more spaces, tested single-word command and failed
        }
        currentCommand = currentCommand.substr(0, lastSpace);
    }

    return CommandResult{
        CommandStatus::NOT_FOUND,
        {"Command not found: " + commandLine}
    };
}

void CommandInterpreter::splitCommand(const std::string& commandLine, std::string& name, std::string& args) {
    size_t spacePos = commandLine.find(' ');
    if (spacePos == std::string::npos) {
        name = commandLine;
        args = "";
    } else {
        name = commandLine.substr(0, spacePos);
        args = commandLine.substr(spacePos + 1);
    }
}
