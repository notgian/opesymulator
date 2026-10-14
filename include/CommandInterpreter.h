#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

enum class CommandStatus {
    SUCCESS,
    EXIT_REQUESTED,
    CLEAR_HISTORY,
    NOT_FOUND,
    CMD_ERROR
};

struct CommandResult {
    CommandStatus status = CommandStatus::SUCCESS;
    std::vector<std::string> output;
};

class CommandInterpreter {
    public:
        using CommandHandler = std::function<CommandResult(const std::string& args)>;
        CommandInterpreter();

        void registerCommand(const std::string& commandName, CommandHandler handler);

        CommandResult executeCommand(const std::string& commandLine);

    private:
        void splitCommand(const std::string& commandLine, std::string& name, std::string& args);
        std::unordered_map<std::string, CommandHandler> handlers_;
};