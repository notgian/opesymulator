#include "CommandInterpreter.h"

CommandInterpreter::CommandInterpreter() {
    // Register default built-in commands
    registerCommand("help", [](const std::string&) {
        return CommandResult{
            CommandStatus::SUCCESS,
            {
                "Available commands:",
                "  help                 - displays this list",
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
    std::string name, args;
    splitCommand(commandLine, name, args);

    if (name.empty()) {
        return CommandResult{CommandStatus::SUCCESS, {}};
    }

    auto it = handlers_.find(name);
    if (it != handlers_.end()) {
        return it->second(args);
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