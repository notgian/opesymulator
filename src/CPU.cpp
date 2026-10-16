#include "CPU.h"
#include <iostream>

CPU::CPU(int numCores) {
    for (int i = 0; i < numCores; ++i) {
        cores_.push_back({i, false, nullptr});
    }
}

bool CPU::assignProcessToCore(int coreId, std::shared_ptr<ProcessControlBlock> pcb) {
    if (coreId >= 0 && coreId < static_cast<int>(cores_.size())) {
        if (!cores_[coreId].isBusy) {
            cores_[coreId].currentProcess = pcb;
            cores_[coreId].isBusy = true;
            pcb->setState(ProcessState::RUNNING);
            return true;
        }
    }
    return false;
}

void CPU::clearCore(int coreId) {
    if (coreId >= 0 && coreId < static_cast<int>(cores_.size())) {
        if (cores_[coreId].currentProcess) {
            cores_[coreId].currentProcess->setState(ProcessState::TERMINATED);
        }
        cores_[coreId].currentProcess = nullptr;
        cores_[coreId].isBusy = false;
    }
}

void CPU::executeCycle() {
    for (int i = 0; i < static_cast<int>(cores_.size()); ++i) {
        auto& core = cores_[i];
        if (core.isBusy && core.currentProcess) {
            auto pcb = core.currentProcess;
            // Check first if process is running
            if (pcb->getState() != ProcessState::RUNNING) {
                continue;
            }
                
            // Execute the command instruction and advance PC
            pcb->executeCurrentCommand(i);
            pcb->moveToNextLine();

            // Check if the process has finished
            if (pcb->hasFinished()) {
                pcb->setState(ProcessState::TERMINATED);
                
                // Release the core
                core.currentProcess = nullptr;
                core.isBusy = false;
            }
        }
    }
}

int CPU::getNumCores() const {
    return static_cast<int>(cores_.size());
}

const CPUCore& CPU::getCore(int coreId) const {
    return cores_.at(coreId);
}

bool CPU::isCoreBusy(int coreId) const {
    return cores_.at(coreId).isBusy;
}