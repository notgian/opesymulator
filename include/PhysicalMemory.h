#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

// Metadata describing a single physical frame in emulated RAM.
struct FrameInfo {
    std::size_t frameIndex = 0;
    bool occupied = false;
    int pid = -1;
    std::size_t virtualPage = 0;
    bool dirty = false;
    bool pinned = false;
    std::uint64_t loadedOrder = 0;
    std::uint64_t lastUsedOrder = 0;
};

// Pre-allocated emulated physical RAM divided into fixed-size frames.
class PhysicalMemory {
public:
    PhysicalMemory(std::size_t totalBytes, std::size_t frameSize);

    // Geometry accessors
    std::size_t getTotalBytes() const;
    std::size_t getFrameSize() const;
    std::size_t getTotalFrames() const;
    std::size_t getUsedFramesCount() const;
    std::size_t getFreeFramesCount() const;
    std::size_t getUsedBytes() const;
    std::size_t getFreeBytes() const;
    std::size_t getProcessResidentFrames(int pid) const;
    std::size_t getProcessResidentBytes(int pid) const;

    // Frame allocation & metadata management (used by MemoryManager)
    int allocateFrame(int pid, std::size_t virtualPage, std::uint64_t order);
    void assignFrame(std::size_t frameIndex, int pid, std::size_t virtualPage, std::uint64_t order);
    void releaseFrame(std::size_t frameIndex);
    void touchFrame(std::size_t frameIndex, std::uint64_t order, bool markDirty = false);
    void setFramePinned(std::size_t frameIndex, bool pinned);

    FrameInfo getFrameInfo(std::size_t frameIndex) const;
    std::vector<FrameInfo> getAllFrameInfo() const;

    // Frame-relative byte and uint16 accessors
    std::uint8_t readByte(std::size_t frameIndex, std::size_t offset) const;
    void writeByte(std::size_t frameIndex, std::size_t offset, std::uint8_t value);

    std::uint16_t readUint16(std::size_t frameIndex, std::size_t offset) const;
    void writeUint16(std::size_t frameIndex, std::size_t offset, std::uint16_t value);

    // Full-frame transfer helpers for paging to/from the backing store
    std::vector<std::uint8_t> readFrame(std::size_t frameIndex) const;
    void writeFrame(std::size_t frameIndex, const std::vector<std::uint8_t>& data);
    void zeroFrame(std::size_t frameIndex);

private:
    void validateFrameAndOffset(std::size_t frameIndex, std::size_t offset, std::size_t accessSize = 1) const;

    std::size_t totalBytes_;
    std::size_t frameSize_;
    std::size_t totalFrames_;

    std::vector<std::uint8_t> memory_;
    std::vector<FrameInfo> frames_;
    mutable std::mutex mutex_;
};
