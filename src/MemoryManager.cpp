#include "MemoryManager.h"
#include "PhysicalMemory.h"
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

MemoryManager::MemoryManager(PhysicalMemory& physicalMemory,
                             const std::string& backingStorePath,
                             PageReplacementPolicy policy)
    : physicalMemory_(physicalMemory),
      backingStorePath_(backingStorePath),
      policy_(policy) {
    initializeBackingStoreFile();
}

std::size_t MemoryManager::computeRequiredPages(std::size_t processMemoryBytes) const {
    const std::size_t frameSize = physicalMemory_.getFrameSize();
    if (processMemoryBytes == 0) return 0;
    return (processMemoryBytes + frameSize - 1) / frameSize;
}

void MemoryManager::setEvictionNotifier(EvictionNotifier notifier) {
    std::lock_guard<std::mutex> lock(memoryMutex_);
    evictionNotifier_ = std::move(notifier);
}

int MemoryManager::ensurePageResident(int pid,
                                      std::size_t virtualPage,
                                      PageTableEntry& entry,
                                      bool isWrite,
                                      bool pinFrame) {
    std::lock_guard<std::mutex> lock(memoryMutex_);
    const std::uint64_t order = ++accessCounter_;

    // Fast path: page is already valid and resident in PhysicalMemory
    if (entry.valid && entry.frameIndex >= 0) {
        const FrameInfo info = physicalMemory_.getFrameInfo(static_cast<std::size_t>(entry.frameIndex));
        if (info.occupied && info.pid == pid && info.virtualPage == virtualPage) {
            physicalMemory_.touchFrame(static_cast<std::size_t>(entry.frameIndex), order, isWrite);
            if (isWrite) entry.dirty = true;
            if (pinFrame) {
                physicalMemory_.setFramePinned(static_cast<std::size_t>(entry.frameIndex), true);
            }
            return entry.frameIndex;
        }
        // Frame was evicted since this entry was last checked
        entry.valid = false;
        entry.frameIndex = -1;
    }

    // Page fault: try allocating a free frame first
    int targetFrame = physicalMemory_.allocateFrame(pid, virtualPage, order);

    // No free frames available: select a victim frame and evict it to the backing store
    if (targetFrame < 0) {
        targetFrame = selectVictimFrameLocked();
        if (targetFrame < 0) {
            return -1; // All frames are currently pinned
        }

        const std::size_t victimIdx = static_cast<std::size_t>(targetFrame);
        const FrameInfo victim = physicalMemory_.getFrameInfo(victimIdx);
        const std::vector<std::uint8_t> victimBytes = physicalMemory_.readFrame(victimIdx);

        // Allocate or reuse a 1-based data line in csopesy-backing-store.txt
        std::size_t slotIndex = 0;
        for (std::size_t i = 0; i < backingSlots_.size(); ++i) {
            if (!backingSlots_[i].inUse) {
                slotIndex = i + 1;
                break;
            }
        }
        if (slotIndex == 0) {
            backingSlots_.push_back({});
            slotIndex = backingSlots_.size();
        }

        BackingSlot& slot = backingSlots_[slotIndex - 1];
        slot.inUse = true;
        slot.pid = victim.pid;
        slot.virtualPage = victim.virtualPage;
        slot.hexLine = bytesToHexLine(victimBytes);

        numPagedOut_.fetch_add(1);
        flushBackingStoreFileLocked();

        if (evictionNotifier_) {
            evictionNotifier_(victim.pid, victim.virtualPage, slotIndex);
        }

        physicalMemory_.assignFrame(victimIdx, pid, virtualPage, order);
    }

    const std::size_t frameIdx = static_cast<std::size_t>(targetFrame);

    // Bring page contents into the frame: either from its indexed line in the backing store
    // or zero-initialized on initial demand allocation.
    if (entry.inBackingStore && entry.backingStoreLine > 0) {
        const std::vector<std::uint8_t> restoredBytes = readPageFromBackingStore(entry.backingStoreLine);
        physicalMemory_.writeFrame(frameIdx, restoredBytes);

        // Free the backing store slot now that the page is back in physical RAM
        if (entry.backingStoreLine <= backingSlots_.size()) {
            BackingSlot& freedSlot = backingSlots_[entry.backingStoreLine - 1];
            freedSlot.inUse = false;
            freedSlot.pid = -1;
            freedSlot.virtualPage = 0;
            freedSlot.hexLine = bytesToHexLine(std::vector<std::uint8_t>(physicalMemory_.getFrameSize(), 0));
            flushBackingStoreFileLocked();
        }
        entry.inBackingStore = false;
        entry.backingStoreLine = 0;
    } else {
        physicalMemory_.zeroFrame(frameIdx);
    }

    numPagedIn_.fetch_add(1);

    entry.valid = true;
    entry.frameIndex = targetFrame;
    entry.dirty = isWrite;
    if (isWrite) {
        physicalMemory_.touchFrame(frameIdx, order, true);
    }
    if (pinFrame) {
        physicalMemory_.setFramePinned(frameIdx, true);
    }

    return targetFrame;
}

void MemoryManager::unpinPage(const PageTableEntry& entry) {
    std::lock_guard<std::mutex> lock(memoryMutex_);
    if (entry.valid && entry.frameIndex >= 0) {
        physicalMemory_.setFramePinned(static_cast<std::size_t>(entry.frameIndex), false);
    }
}

std::uint8_t MemoryManager::readVirtualByte(int pid,
                                            std::size_t virtualAddress,
                                            std::vector<PageTableEntry>& pageTable) {
    const std::size_t frameSize = physicalMemory_.getFrameSize();
    const std::size_t virtualPage = virtualAddress / frameSize;
    const std::size_t offset = virtualAddress % frameSize;

    if (virtualPage >= pageTable.size()) {
        throw std::out_of_range("Virtual memory access violation.");
    }

    const int frameIdx = ensurePageResident(pid, virtualPage, pageTable[virtualPage], /*isWrite=*/false);
    if (frameIdx < 0) {
        throw std::runtime_error("Failed to resolve page fault during read.");
    }
    return physicalMemory_.readByte(static_cast<std::size_t>(frameIdx), offset);
}

void MemoryManager::writeVirtualByte(int pid,
                                     std::size_t virtualAddress,
                                     std::uint8_t value,
                                     std::vector<PageTableEntry>& pageTable) {
    const std::size_t frameSize = physicalMemory_.getFrameSize();
    const std::size_t virtualPage = virtualAddress / frameSize;
    const std::size_t offset = virtualAddress % frameSize;

    if (virtualPage >= pageTable.size()) {
        throw std::out_of_range("Virtual memory access violation.");
    }

    const int frameIdx = ensurePageResident(pid, virtualPage, pageTable[virtualPage], /*isWrite=*/true);
    if (frameIdx < 0) {
        throw std::runtime_error("Failed to resolve page fault during write.");
    }
    physicalMemory_.writeByte(static_cast<std::size_t>(frameIdx), offset, value);
}

std::uint16_t MemoryManager::readVirtualUint16(int pid,
                                               std::size_t virtualAddress,
                                               std::vector<PageTableEntry>& pageTable) {
    const std::size_t frameSize = physicalMemory_.getFrameSize();
    const std::size_t firstPage = virtualAddress / frameSize;
    const std::size_t offset = virtualAddress % frameSize;

    // Fast path: both bytes fit inside the same page/frame
    if (offset + 2 <= frameSize) {
        if (firstPage >= pageTable.size()) {
            throw std::out_of_range("Virtual memory access violation.");
        }
        const int frameIdx = ensurePageResident(pid, firstPage, pageTable[firstPage], /*isWrite=*/false);
        if (frameIdx < 0) {
            throw std::runtime_error("Failed to resolve page fault during uint16 read.");
        }
        return physicalMemory_.readUint16(static_cast<std::size_t>(frameIdx), offset);
    }

    // Page-crossing uint16: pin first page while bringing in second page so it cannot be evicted mid-read
    const std::size_t secondPage = (virtualAddress + 1) / frameSize;
    if (secondPage >= pageTable.size()) {
        throw std::out_of_range("Virtual memory access violation.");
    }

    const int firstFrame = ensurePageResident(pid, firstPage, pageTable[firstPage], /*isWrite=*/false, /*pinFrame=*/true);
    if (firstFrame < 0) {
        throw std::runtime_error("Failed to resolve page fault on first page of uint16 read.");
    }

    int secondFrame = -1;
    try {
        secondFrame = ensurePageResident(pid, secondPage, pageTable[secondPage], /*isWrite=*/false);
    } catch (...) {
        unpinPage(pageTable[firstPage]);
        throw;
    }

    const std::uint16_t low = physicalMemory_.readByte(static_cast<std::size_t>(firstFrame), offset);
    unpinPage(pageTable[firstPage]);

    if (secondFrame < 0) {
        throw std::runtime_error("Failed to resolve page fault on second page of uint16 read.");
    }
    const std::uint16_t high = physicalMemory_.readByte(static_cast<std::size_t>(secondFrame), 0);
    return static_cast<std::uint16_t>(low | (high << 8));
}

void MemoryManager::writeVirtualUint16(int pid,
                                       std::size_t virtualAddress,
                                       std::uint16_t value,
                                       std::vector<PageTableEntry>& pageTable) {
    const std::size_t frameSize = physicalMemory_.getFrameSize();
    const std::size_t firstPage = virtualAddress / frameSize;
    const std::size_t offset = virtualAddress % frameSize;

    // Fast path: both bytes fit inside the same page/frame
    if (offset + 2 <= frameSize) {
        if (firstPage >= pageTable.size()) {
            throw std::out_of_range("Virtual memory access violation.");
        }
        const int frameIdx = ensurePageResident(pid, firstPage, pageTable[firstPage], /*isWrite=*/true);
        if (frameIdx < 0) {
            throw std::runtime_error("Failed to resolve page fault during uint16 write.");
        }
        physicalMemory_.writeUint16(static_cast<std::size_t>(frameIdx), offset, value);
        return;
    }

    // Page-crossing uint16: pin first page while bringing in second page
    const std::size_t secondPage = (virtualAddress + 1) / frameSize;
    if (secondPage >= pageTable.size()) {
        throw std::out_of_range("Virtual memory access violation.");
    }

    const int firstFrame = ensurePageResident(pid, firstPage, pageTable[firstPage], /*isWrite=*/true, /*pinFrame=*/true);
    if (firstFrame < 0) {
        throw std::runtime_error("Failed to resolve page fault on first page of uint16 write.");
    }

    int secondFrame = -1;
    try {
        secondFrame = ensurePageResident(pid, secondPage, pageTable[secondPage], /*isWrite=*/true);
    } catch (...) {
        unpinPage(pageTable[firstPage]);
        throw;
    }

    physicalMemory_.writeByte(static_cast<std::size_t>(firstFrame), offset, static_cast<std::uint8_t>(value & 0xFFu));
    unpinPage(pageTable[firstPage]);

    if (secondFrame < 0) {
        throw std::runtime_error("Failed to resolve page fault on second page of uint16 write.");
    }
    physicalMemory_.writeByte(static_cast<std::size_t>(secondFrame), 0, static_cast<std::uint8_t>((value >> 8) & 0xFFu));
}

void MemoryManager::releaseProcessMemory(int pid, std::vector<PageTableEntry>& pageTable) {
    std::lock_guard<std::mutex> lock(memoryMutex_);
    bool backingModified = false;

    for (std::size_t vp = 0; vp < pageTable.size(); ++vp) {
        PageTableEntry& entry = pageTable[vp];
        if (entry.valid && entry.frameIndex >= 0) {
            const std::size_t frameIdx = static_cast<std::size_t>(entry.frameIndex);
            const FrameInfo info = physicalMemory_.getFrameInfo(frameIdx);
            if (info.occupied && info.pid == pid && info.virtualPage == vp) {
                physicalMemory_.releaseFrame(frameIdx);
            }
        }

        if (entry.inBackingStore && entry.backingStoreLine > 0 && entry.backingStoreLine <= backingSlots_.size()) {
            BackingSlot& slot = backingSlots_[entry.backingStoreLine - 1];
            if (slot.inUse && slot.pid == pid) {
                slot.inUse = false;
                slot.pid = -1;
                slot.virtualPage = 0;
                slot.hexLine = bytesToHexLine(std::vector<std::uint8_t>(physicalMemory_.getFrameSize(), 0));
                backingModified = true;
            }
        }

        entry.valid = false;
        entry.frameIndex = -1;
        entry.inBackingStore = false;
        entry.backingStoreLine = 0;
        entry.dirty = false;
    }

    if (backingModified) {
        flushBackingStoreFileLocked();
    }
}

std::size_t MemoryManager::writePageToBackingStore(int pid,
                                                   std::size_t virtualPage,
                                                   const std::vector<std::uint8_t>& pageBytes,
                                                   std::size_t existingLine) {
    std::lock_guard<std::mutex> lock(memoryMutex_);
    if (pageBytes.size() != physicalMemory_.getFrameSize()) {
        throw std::invalid_argument("Page size mismatch when writing to backing store.");
    }

    std::size_t lineIndex = existingLine;
    if (lineIndex == 0 || lineIndex > backingSlots_.size()) {
        for (std::size_t i = 0; i < backingSlots_.size(); ++i) {
            if (!backingSlots_[i].inUse) {
                lineIndex = i + 1;
                break;
            }
        }
        if (lineIndex == 0) {
            backingSlots_.push_back({});
            lineIndex = backingSlots_.size();
        }
    }

    BackingSlot& slot = backingSlots_[lineIndex - 1];
    slot.inUse = true;
    slot.pid = pid;
    slot.virtualPage = virtualPage;
    slot.hexLine = bytesToHexLine(pageBytes);

    exit(233);

    flushBackingStoreFileLocked();
    return lineIndex;
}

std::vector<std::uint8_t> MemoryManager::readPageFromBackingStore(std::size_t lineIndex) const {
    if (lineIndex == 0) {
        throw std::out_of_range("Line 0 is reserved for paging metadata in the backing store.");
    }

    std::ifstream inFile(backingStorePath_);
    if (!inFile.is_open()) {
        throw std::runtime_error("Unable to open backing store file: " + backingStorePath_);
    }

    // Line 0 is the paging information header; data lines start at index 1.
    std::string line;
    std::size_t currentLine = 0;
    while (std::getline(inFile, line)) {
        if (currentLine == lineIndex) {
            return hexLineToBytes(line, physicalMemory_.getFrameSize());
        }
        ++currentLine;
    }

    throw std::out_of_range("Backing store line index out of range: " + std::to_string(lineIndex));
}

MemoryStatsSnapshot MemoryManager::getStats() const {
    MemoryStatsSnapshot stats;
    stats.totalMemoryBytes = physicalMemory_.getTotalBytes();
    stats.frameSizeBytes = physicalMemory_.getFrameSize();
    stats.totalFrames = physicalMemory_.getTotalFrames();
    stats.usedFrames = physicalMemory_.getUsedFramesCount();
    stats.freeFrames = stats.totalFrames - stats.usedFrames;
    stats.usedMemoryBytes = stats.usedFrames * stats.frameSizeBytes;
    stats.freeMemoryBytes = stats.freeFrames * stats.frameSizeBytes;
    stats.numPagedIn = numPagedIn_.load();
    stats.numPagedOut = numPagedOut_.load();
    return stats;
}

std::uint64_t MemoryManager::getNumPagedIn() const {
    return numPagedIn_.load();
}

std::uint64_t MemoryManager::getNumPagedOut() const {
    return numPagedOut_.load();
}

std::size_t MemoryManager::getProcessMemoryUsage(int pid) const {
    return physicalMemory_.getProcessResidentBytes(pid);
}

PhysicalMemory& MemoryManager::getPhysicalMemory() const {
   return physicalMemory_;
}

int MemoryManager::selectVictimFrameLocked() const {
    const std::vector<FrameInfo> frames = physicalMemory_.getAllFrameInfo();
    int victimIndex = -1;
    std::uint64_t bestMetric = std::numeric_limits<std::uint64_t>::max();

    for (const auto& frame : frames) {
        if (!frame.occupied || frame.pinned) continue;

        const std::uint64_t metric = (policy_ == PageReplacementPolicy::LRU)
            ? frame.lastUsedOrder
            : frame.loadedOrder;

        if (metric < bestMetric) {
            bestMetric = metric;
            victimIndex = static_cast<int>(frame.frameIndex);
        }
    }

    return victimIndex;
}

void MemoryManager::initializeBackingStoreFile() {
    std::lock_guard<std::mutex> lock(memoryMutex_);
    backingSlots_.clear();
    flushBackingStoreFileLocked();
}

void MemoryManager::flushBackingStoreFileLocked() const {
    std::ofstream outFile(backingStorePath_, std::ios::trunc);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to write backing store file: " + backingStorePath_);
    }

    std::size_t activeStoredPages = 0;
    for (const auto& slot : backingSlots_) {
        if (slot.inUse) ++activeStoredPages;
    }

    // Line 0 (first line): Paging configuration & runtime state
    outFile << "max-overall-mem=" << physicalMemory_.getTotalBytes()
            << " mem-per-frame=" << physicalMemory_.getFrameSize()
            << " total-frames=" << physicalMemory_.getTotalFrames()
            << " stored-pages=" << activeStoredPages
            << " paged-in=" << numPagedIn_.load()
            << " paged-out=" << numPagedOut_.load()
            << "\n";

    // Lines 1..N: Exact hex bytes per stored page
    for (const auto& slot : backingSlots_) {
        outFile << slot.hexLine << "\n";
    }
}

std::string MemoryManager::bytesToHexLine(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) oss << ' ';
        oss << std::setw(2) << static_cast<unsigned int>(bytes[i]);
    }
    return oss.str();
}

std::vector<std::uint8_t> MemoryManager::hexLineToBytes(const std::string& line, std::size_t expectedBytes) {
    std::vector<std::uint8_t> result;
    result.reserve(expectedBytes);

    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        unsigned int byteVal = std::stoul(token, nullptr, 16);
        result.push_back(static_cast<std::uint8_t>(byteVal & 0xFFu));
    }

    if (result.size() != expectedBytes) {
        throw std::runtime_error("Corrupted backing store line: expected " +
                                 std::to_string(expectedBytes) + " hex bytes, got " +
                                 std::to_string(result.size()));
    }

    return result;
}
