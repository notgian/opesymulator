#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include "PhysicalMemory.h"

enum class PageReplacementPolicy {
    FIFO,
    LRU
};

// Virtual-to-physical reference stored in a process's future page table.
// When valid == true, frameIndex directly identifies the physical frame in PhysicalMemory
// (starting at byte offset frameIndex * frameSize).
// When valid == false and inBackingStore == true, backingStoreLine indexes the 1-based
// data line in csopesy-backing-store.txt where the page's hex bytes are stored.
struct PageTableEntry {
    bool valid = false;
    int frameIndex = -1;
    bool inBackingStore = false;
    std::size_t backingStoreLine = 0; // 1-based data line index (line 0 is the header)
    bool dirty = false;

    std::size_t physicalBaseAddress(std::size_t frameSize) const {
        return frameIndex >= 0 ? static_cast<std::size_t>(frameIndex) * frameSize : 0;
    }
};

struct MemoryStatsSnapshot {
    std::size_t totalMemoryBytes = 0;
    std::size_t usedMemoryBytes = 0;
    std::size_t freeMemoryBytes = 0;
    std::size_t frameSizeBytes = 0;
    std::size_t totalFrames = 0;
    std::size_t usedFrames = 0;
    std::size_t freeFrames = 0;
    std::uint64_t numPagedIn = 0;
    std::uint64_t numPagedOut = 0;
};

class MemoryManager {
public:
    static constexpr const char* DEFAULT_BACKING_STORE_PATH = "csopesy-backing-store.txt";

    // Callback used when evicting another process's frame so its PageTableEntry is invalidated
    // and updated with its new backing store line index.
    using EvictionNotifier = std::function<void(int victimPid, std::size_t victimVirtualPage, std::size_t backingStoreLine)>;

    MemoryManager(PhysicalMemory& physicalMemory,
                  const std::string& backingStorePath = DEFAULT_BACKING_STORE_PATH,
                  PageReplacementPolicy policy = PageReplacementPolicy::FIFO);

    // Helper for computing the number of pages P = ceil(M / mem-per-frame)
    std::size_t computeRequiredPages(std::size_t processMemoryBytes) const;

    // Optional hook for updating a victim process's PageTableEntry upon eviction
    void setEvictionNotifier(EvictionNotifier notifier);

    // Ensures `entry` for `(pid, virtualPage)` is resident in PhysicalMemory.
    // Handles page faults on demand (allocating a free frame or evicting a victim frame to
    // the backing store and paging in from the indexed line if previously swapped).
    // Returns the valid physical frame index in PhysicalMemory (or -1 if no frame could be acquired).
    int ensurePageResident(int pid,
                           std::size_t virtualPage,
                           PageTableEntry& entry,
                           bool isWrite = false,
                           bool pinFrame = false);

    // Releases temporary frame pinning after a multi-page access completes
    void unpinPage(const PageTableEntry& entry);

    // Virtual-memory byte and uint16 accessors operating through a process's PageTableEntry vector
    std::uint8_t readVirtualByte(int pid,
                                 std::size_t virtualAddress,
                                 std::vector<PageTableEntry>& pageTable);

    void writeVirtualByte(int pid,
                          std::size_t virtualAddress,
                          std::uint8_t value,
                          std::vector<PageTableEntry>& pageTable);

    std::uint16_t readVirtualUint16(int pid,
                                    std::size_t virtualAddress,
                                    std::vector<PageTableEntry>& pageTable);

    void writeVirtualUint16(int pid,
                            std::size_t virtualAddress,
                            std::uint16_t value,
                            std::vector<PageTableEntry>& pageTable);

    // Frees all physical frames and backing store slots owned by a terminated process
    void releaseProcessMemory(int pid, std::vector<PageTableEntry>& pageTable);

    // Direct backing store line operations (line 0 = paging header; lines 1..N = hex page data)
    std::size_t writePageToBackingStore(int pid,
                                        std::size_t virtualPage,
                                        const std::vector<std::uint8_t>& pageBytes,
                                        std::size_t existingLine = 0);

    std::vector<std::uint8_t> readPageFromBackingStore(std::size_t lineIndex) const;

    // Metrics for vmstat and process-smi
    MemoryStatsSnapshot getStats() const;
    std::uint64_t getNumPagedIn() const;
    std::uint64_t getNumPagedOut() const;
    std::size_t getProcessMemoryUsage(int pid) const;

    PhysicalMemory& getPhysicalMemory() const;

private:
    struct BackingSlot {
        bool inUse = false;
        int pid = -1;
        std::size_t virtualPage = 0;
        std::string hexLine;
    };

    int selectVictimFrameLocked() const;
    void initializeBackingStoreFile();
    void flushBackingStoreFileLocked() const;

    static std::string bytesToHexLine(const std::vector<std::uint8_t>& bytes);
    static std::vector<std::uint8_t> hexLineToBytes(const std::string& line, std::size_t expectedBytes);

    PhysicalMemory& physicalMemory_;
    std::string backingStorePath_;
    PageReplacementPolicy policy_;
    EvictionNotifier evictionNotifier_;

    mutable std::mutex memoryMutex_;
    std::atomic<std::uint64_t> accessCounter_{0};
    std::atomic<std::uint64_t> numPagedIn_{0};
    std::atomic<std::uint64_t> numPagedOut_{0};

    // Index 0 corresponds to data line 1 in csopesy-backing-store.txt (after line 0 header)
    std::vector<BackingSlot> backingSlots_;
};
