#include "AddCommand.h"

AddCommand::AddCommand(int pid, const std::string& dest, const Operand& lhs, const Operand& rhs)
    : ICommand(pid, ICommandType::ADD), dest_(dest), lhs_(lhs), rhs_(rhs) {}

int AddCommand::execute(SymbolTable& symbolTable) {
    long long sum = static_cast<long long>(lhs_.resolve(symbolTable)) + rhs_.resolve(symbolTable);
    symbolTable.setVariable(dest_, clampUint16(sum));   // clamps at 65535
    return 1;
}
