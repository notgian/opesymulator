#include "ProcessControlBlock.h"

ProcessControlBlock::ProcessControlBlock(int pid, const std::string& name)
    : pid_(pid),
      name_(name),
      state_(ProcessState::READY),
      programCounter_(0) {
}

void ProcessControlBlock::addCommand(std::shared_ptr<ICommand> command) {
    commandList.push_back(command);
}

// TODO: may need to revisit
void ProcessControlBlock::executeCurrentCommand() {
    if (programCounter_ >= 0 && programCounter_ < static_cast<int>(commandList.size())) {
        if (commandList[programCounter_]) {
            // Note: Adjust the execute signature depending on your ICommand interface 
            // (e.g., passing symbolTable if commands need access to process variables)
            // commandList[programCounter_]->execute(symbolTable);
            
            commandList[programCounter_]->execute(); 
        }
    }
}

void ProcessControlBlock::moveToNextLine() {
    if (programCounter_ < static_cast<int>(commandList.size())) {
        programCounter_++;
    }
}

int ProcessControlBlock::getPID() const {
    return pid_;
}

const std::string& ProcessControlBlock::getName() const {
    return name_;
}

ProcessState ProcessControlBlock::getState() const {
    return state_;
}

int ProcessControlBlock::getProgramCounter() const {
    return programCounter_;
}

void ProcessControlBlock::setState(ProcessState state) {
    this->state_ = state;
}

void ProcessControlBlock::setProgramCounter(int pc) {
    this->programCounter_ = pc;
}