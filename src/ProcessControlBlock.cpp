#include "ProcessControlBlock.h"
#include "TimeUtil.h"
#include <algorithm>

ProcessControlBlock::ProcessControlBlock(int pid,
                                         const std::string& name,
                                         std::size_t allocatedMemoryBytes,
                                         MemoryManager* memoryManager)
    : pid_(pid),
      name_(name),
      creationTime_(currentTimestamp()),
      allocatedMemoryBytes_(allocatedMemoryBytes),
      memoryManager_(memoryManager),
      state_(ProcessState::READY),
      programCounter_(0) {
    const std::size_t numPages = memoryManager_
        ? memoryManager_->computeRequiredPages(allocatedMemoryBytes_)
        : std::max<std::size_t>(1, allocatedMemoryBytes_ / SymbolTable::SEGMENT_SIZE_BYTES);

    pageTable_.resize(std::max<std::size_t>(1, numPages));
    symbolTable.bindMemory(pid_, memoryManager_, &pageTable_);
}

void ProcessControlBlock::addCommand(std::shared_ptr<ICommand> command) {
    if (command) {
        command->setParentProcess(this);
    }
    commandList.push_back(command);
}

int ProcessControlBlock::executeCurrentCommand(int coreId) {
    if (programCounter_ >= 0 && programCounter_ < static_cast<int>(commandList.size())) {
        if (commandList[programCounter_]) {
            currentCoreId_.store(coreId);
            return commandList[programCounter_]->execute(symbolTable);
        }
    }

    return 0;
}

void ProcessControlBlock::jumpLines(int jump) {
    if (jump == 0) {
        return;
    }

    if (programCounter_.load() < static_cast<int>(commandList.size())) {
        programCounter_ += jump;
    }
}

bool ProcessControlBlock::hasFinished() const {
    return programCounter_ >= static_cast<int>(commandList.size());
}

int ProcessControlBlock::getPID() const {
    return pid_;
}

const std::string& ProcessControlBlock::getName() const {
    return name_;
}

ProcessState ProcessControlBlock::getState() const {
    return state_.load();
}

int ProcessControlBlock::getTotalCommands() const {
    return static_cast<int>(commandList.size());
}

const std::string& ProcessControlBlock::getCreationTime() const {
    return creationTime_;
}

int ProcessControlBlock::getProgramCounter() const {
    return programCounter_;
}

void ProcessControlBlock::setState(ProcessState state) {
    state_.store(state);
}

void ProcessControlBlock::setProgramCounter(int pc) {
    programCounter_.store(pc);
}

void ProcessControlBlock::setCurrentCoreId(int coreId) {
    currentCoreId_.store(coreId);
}

int ProcessControlBlock::getCurrentCoreId() const {
    return currentCoreId_.load();
}

std::size_t ProcessControlBlock::getAllocatedMemory() const {
    return allocatedMemoryBytes_;
}

std::size_t ProcessControlBlock::getNumPages() const {
    return pageTable_.size();
}

std::vector<PageTableEntry>& ProcessControlBlock::getPageTable() {
    return pageTable_;
}

const std::vector<PageTableEntry>& ProcessControlBlock::getPageTable() const {
    return pageTable_;
}

SymbolTable& ProcessControlBlock::getSymbolTable() {
    return symbolTable;
}

void ProcessControlBlock::onPageEvicted(std::size_t virtualPage, std::size_t backingStoreLine) {
    if (virtualPage < pageTable_.size()) {
        pageTable_[virtualPage].valid = false;
        pageTable_[virtualPage].frameIndex = -1;
        pageTable_[virtualPage].inBackingStore = true;
        pageTable_[virtualPage].backingStoreLine = backingStoreLine;
        pageTable_[virtualPage].dirty = false;
    }
}

void ProcessControlBlock::releaseMemory() {
    if (memoryManager_) {
        memoryManager_->releaseProcessMemory(pid_, pageTable_);
    }
}

void ProcessControlBlock::setSleepTicks(int ticks) {
    sleepTicks_.store(ticks);
}

int ProcessControlBlock::getSleepTicks() const {
    return sleepTicks_.load();
}

void ProcessControlBlock::clearSleep() {
    sleepTicks_.store(0);
}
