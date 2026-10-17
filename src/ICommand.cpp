#include "ICommand.h"


ICommand::ICommand(int pid, ICommandType commandType) : pid_(pid), commandType_(commandType) {

};

ICommandType ICommand::getCommandType() const {
    return this->commandType_;
}