#pragma once

enum class ICommandType {
    IO,
    PRINT
};

class ICommand {
    public:
        ICommand(int pid, ICommandType CommandType);
        ICommandType getCommandType();
        virtual void execute();

    protected:
        int pid_;
        ICommandType commandType_;
};