#include "ICommand.h"


ICommand::ICommand(int pid, ICommandType commandType) : pid_(pid), commandType_(commandType), parentProcess_(nullptr) {

};

ICommandType ICommand::getCommandType() const {
    return this->commandType_;
}

ProcessControlBlock* ICommand::getParentProcess() const {
    return this->parentProcess_;
}

void ICommand::setParentProcess(ProcessControlBlock* parent) {
    this->parentProcess_ = parent;
}