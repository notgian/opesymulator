#include "ForCommand.h"
#include "TimeUtil.h"
#include "ProcessScreen.h"
#include <fstream>

ForCommand::ForCommand(int pid, int repeats)
    : ICommand(pid, ICommandType::LOOP), repeats_(repeats) {}


void ForCommand::addCommand(std::shared_ptr<ICommand> command) {
    if (this->getParentProcess() == nullptr) {
        throw std::runtime_error("Parent process is not defined. Add the FOR command to a PCB first before adding commands to it.");
    }
    // Check if command is an instance of ForCommand
    bool commandIsFor = false;
    if (auto forCmd = std::dynamic_pointer_cast<ForCommand>(command)) {
        commandIsFor = true;
        // Prevent nesting beyond the max level
        if (this->nestingLevel_ >= MAXIMUM_FOR_NESTING)
            throw std::runtime_error("Maximum FOR loop nesting level exceeded.");

        forCmd->nestingLevel_ = this->nestingLevel_ + 1;
    }

    if (command) {
        command->setParentProcess(this->getParentProcess());

        for (auto instr: commandList_) {
            instr->setParentProcess(this->getParentProcess());
        }
    }
    commandList_.push_back(command);
}

int ForCommand::execute(SymbolTable& symbolTable) {
    // Guard against empty command list or completed loop
    if (commandList_.empty() || isFinished()) {
        return 1;
    }

    // Bounds safety check before indexing
    if (internalPC_ >= commandList_.size()) {
        internalPC_ = 0;
        currentRepeat_++;
    }
    if (isFinished()) {
        reset_();
        return 1;
    }
    
    // Execute the current command that is in the for loop
    int jump = commandList_.at(internalPC_)->execute(symbolTable);
    internalPC_ += jump;

    // Check if the loop has finished all repeats
    if (isFinished()) {
        reset_();
        return 1;
    }

    // Move to next loop if needed.
    else if (internalPC_ >= commandList_.size()) {
        internalPC_ = 0;
        currentRepeat_++;
    }

    return 0;
}

bool ForCommand::isFinished() {
    if (commandList_.empty()) {
        return true;
    }

    if ((internalPC_ >= commandList_.size() && currentRepeat_ >= repeats_) || currentRepeat_ > repeats_)
        return true;

    return false;
}

int ForCommand::getCurrentRepeat() {
    return currentRepeat_;
}

void ForCommand::reset_() {
    internalPC_ = 0;
    currentRepeat_ = 1;
}