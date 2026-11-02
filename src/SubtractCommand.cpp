#include "SubtractCommand.h"

SubtractCommand::SubtractCommand(int pid, const std::string& dest, const Operand& lhs, const Operand& rhs)
    : ICommand(pid, ICommandType::SUBTRACT), dest_(dest), lhs_(lhs), rhs_(rhs) {}

int SubtractCommand::execute(SymbolTable& symbolTable) {
    long long diff = static_cast<long long>(lhs_.resolve(symbolTable)) - rhs_.resolve(symbolTable);
    symbolTable.setVariable(dest_, clampUint16(diff));  // clamps at 0
    return 1;
}
