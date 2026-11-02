#include "DeclareCommand.h"
#include "Operand.h"

DeclareCommand::DeclareCommand(int pid, const std::string& var, int value)
    : ICommand(pid, ICommandType::DECLARE), var_(var), value_(clampUint16(value)) {}

int DeclareCommand::execute(SymbolTable& symbolTable) {
    symbolTable.setVariable(var_, value_);
    return 1;
}
