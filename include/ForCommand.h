#pragma once

#include "ICommand.h"
#include "SymbolTable.h"

#include <memory>
#include <vector>
#include <string>

constexpr int MAXIMUM_FOR_NESTING = 3;


class ForCommand : public ICommand {
public:
    ForCommand(int pid, int repeats);

    void addCommand(std::shared_ptr<ICommand> command);

    int execute(SymbolTable& symbolTable) override;

    bool isFinished();

    int getCurrentRepeat();

private:
    std::vector<std::shared_ptr<ICommand>> commandList_;
    int repeats_;

    int internalPC_ = 0;
    int currentRepeat_ = 1;

    int nestingLevel_ = 1;

    void reset_();
};