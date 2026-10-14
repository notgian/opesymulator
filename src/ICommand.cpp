#include "ICommand.h"


ICommand::ICommand(int pid, ICommandType commandType) : pid_(pid), commandType_(commandType) {

};

ICommandType ICommand::getCommandType() {
    return this->commandType_;
}

void ICommand::execute() {
    // Do something
}