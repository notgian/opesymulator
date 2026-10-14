#pragma once

#include <memory>
#include <vector>
#include "ProcessControlBlock.h"

struct CPUCore {
    int coreId;
    bool isBusy = false;
    std::shared_ptr<ProcessControlBlock> currentProcess = nullptr;
};

class CPU {
public:
    CPU(int numCores = 4);

    // Core management & scheduling assignment
    bool assignProcessToCore(int coreId, std::shared_ptr<ProcessControlBlock> pcb);
    void clearCore(int coreId);
    
    void executeCycle();

    // Accessors
    int getNumCores() const;
    const CPUCore& getCore(int coreId) const;
    bool isCoreBusy(int coreId) const;

private:
    std::vector<CPUCore> cores_;
};