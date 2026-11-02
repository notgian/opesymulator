#pragma once

#include <cstdint>
#include <string>
#include "SymbolTable.h"

constexpr int UINT16_MAX_VALUE = 65535;

// Clamps any int to the uint16 range [0, 65535] as required by the specs.
inline int clampUint16(long long value) {
    if (value < 0) return 0;
    if (value > UINT16_MAX_VALUE) return UINT16_MAX_VALUE;
    return static_cast<int>(value);
}

// An instruction argument that is either a variable name or a uint16 literal.
class Operand {
public:
    static Operand variable(const std::string& name) { return Operand(true, name, 0); }
    static Operand literal(int value) { return Operand(false, "", clampUint16(value)); }

    // Undeclared variables are auto-declared with 0, as the specs say.
    int resolve(SymbolTable& symbolTable) const {
        if (!isVariable_) return value_;
        if (!symbolTable.hasVariable(name_)) symbolTable.setVariable(name_, 0);
        return symbolTable.getVariable(name_);
    }

    bool isVariable() const { return isVariable_; }
    const std::string& name() const { return name_; }

private:
    Operand(bool isVar, const std::string& name, int value)
        : isVariable_(isVar), name_(name), value_(value) {}

    bool isVariable_;
    std::string name_;
    int value_;
};
