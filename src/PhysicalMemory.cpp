#include "PhysicalMemory.h"
#include <algorithm>
#include <stdexcept>

PhysicalMemory::PhysicalMemory(std::size_t totalBytes, std::size_t frameSize)
    : totalBytes_(totalBytes),
      frameSize_(frameSize),
      totalFrames_(frameSize > 0 ? totalBytes / frameSize : 0) {
    if (frameSize_ == 0 || totalFrames_ == 0 || (totalBytes_ % frameSize_) != 0) {
        throw std::invalid_argument("Invalid PhysicalMemory dimensions.");
    }

    // Pre-allocate the entire emulated physical memory space upon startup
    memory_.assign(totalBytes_, 0);
    frames_.resize(totalFrames_);
    for (std::size_t i = 0; i < totalFrames_; ++i) {
        frames_[i].frameIndex = i;
    }
}

std::size_t PhysicalMemory::getTotalBytes() const {
    return totalBytes_;
}

std::size_t PhysicalMemory::getFrameSize() const {
    return frameSize_;
}

std::size_t PhysicalMemory::getTotalFrames() const {
    return totalFrames_;
}

std::size_t PhysicalMemory::getUsedFramesCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t used = 0;
    for (const auto& frame : frames_) {
        if (frame.occupied) ++used;
    }
    return used;
}

std::size_t PhysicalMemory::getFreeFramesCount() const {
    return totalFrames_ - getUsedFramesCount();
}

std::size_t PhysicalMemory::getUsedBytes() const {
    return getUsedFramesCount() * frameSize_;
}

std::size_t PhysicalMemory::getFreeBytes() const {
    return getFreeFramesCount() * frameSize_;
}

std::size_t PhysicalMemory::getProcessResidentFrames(int pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t count = 0;
    for (const auto& frame : frames_) {
        if (frame.occupied && frame.pid == pid) {
            ++count;
        }
    }
    return count;
}

std::size_t PhysicalMemory::getProcessResidentBytes(int pid) const {
    return getProcessResidentFrames(pid) * frameSize_;
}

int PhysicalMemory::allocateFrame(int pid, std::size_t virtualPage, std::uint64_t order) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (std::size_t i = 0; i < totalFrames_; ++i) {
        if (!frames_[i].occupied) {
            frames_[i].occupied = true;
            frames_[i].pid = pid;
            frames_[i].virtualPage = virtualPage;
            frames_[i].dirty = false;
            frames_[i].pinned = false;
            frames_[i].loadedOrder = order;
            frames_[i].lastUsedOrder = order;
            return static_cast<int>(i);
        }
    }
    return -1; // Physical memory is full
}

void PhysicalMemory::assignFrame(std::size_t frameIndex, int pid, std::size_t virtualPage, std::uint64_t order) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Frame index out of range.");
    }
    frames_[frameIndex].occupied = true;
    frames_[frameIndex].pid = pid;
    frames_[frameIndex].virtualPage = virtualPage;
    frames_[frameIndex].dirty = false;
    frames_[frameIndex].pinned = false;
    frames_[frameIndex].loadedOrder = order;
    frames_[frameIndex].lastUsedOrder = order;
}

void PhysicalMemory::releaseFrame(std::size_t frameIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Frame index out of range.");
    }
    frames_[frameIndex].occupied = false;
    frames_[frameIndex].pid = -1;
    frames_[frameIndex].virtualPage = 0;
    frames_[frameIndex].dirty = false;
    frames_[frameIndex].pinned = false;
    frames_[frameIndex].loadedOrder = 0;
    frames_[frameIndex].lastUsedOrder = 0;

    const std::size_t base = frameIndex * frameSize_;
    std::fill(memory_.begin() + base, memory_.begin() + base + frameSize_, 0);
}

void PhysicalMemory::touchFrame(std::size_t frameIndex, std::uint64_t order, bool markDirty) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Frame index out of range.");
    }
    frames_[frameIndex].lastUsedOrder = order;
    if (markDirty) {
        frames_[frameIndex].dirty = true;
    }
}

void PhysicalMemory::setFramePinned(std::size_t frameIndex, bool pinned) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Frame index out of range.");
    }
    frames_[frameIndex].pinned = pinned;
}

FrameInfo PhysicalMemory::getFrameInfo(std::size_t frameIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Frame index out of range.");
    }
    return frames_[frameIndex];
}

std::vector<FrameInfo> PhysicalMemory::getAllFrameInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_;
}

std::uint8_t PhysicalMemory::readByte(std::size_t frameIndex, std::size_t offset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    validateFrameAndOffset(frameIndex, offset, 1);
    return memory_[frameIndex * frameSize_ + offset];
}

void PhysicalMemory::writeByte(std::size_t frameIndex, std::size_t offset, std::uint8_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    validateFrameAndOffset(frameIndex, offset, 1);
    memory_[frameIndex * frameSize_ + offset] = value;
    frames_[frameIndex].dirty = true;
}

std::uint16_t PhysicalMemory::readUint16(std::size_t frameIndex, std::size_t offset) const {
    std::lock_guard<std::mutex> lock(mutex_);
    validateFrameAndOffset(frameIndex, offset, 2);
    const std::size_t physAddr = frameIndex * frameSize_ + offset;
    const std::uint16_t low = memory_[physAddr];
    const std::uint16_t high = memory_[physAddr + 1];
    return static_cast<std::uint16_t>(low | (high << 8));
}

void PhysicalMemory::writeUint16(std::size_t frameIndex, std::size_t offset, std::uint16_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    validateFrameAndOffset(frameIndex, offset, 2);
    const std::size_t physAddr = frameIndex * frameSize_ + offset;
    memory_[physAddr] = static_cast<std::uint8_t>(value & 0xFFu);
    memory_[physAddr + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    frames_[frameIndex].dirty = true;
}

std::vector<std::uint8_t> PhysicalMemory::readFrame(std::size_t frameIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Frame index out of range.");
    }
    const std::size_t base = frameIndex * frameSize_;
    return std::vector<std::uint8_t>(memory_.begin() + base, memory_.begin() + base + frameSize_);
}

void PhysicalMemory::writeFrame(std::size_t frameIndex, const std::vector<std::uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Frame index out of range.");
    }
    if (data.size() != frameSize_) {
        throw std::invalid_argument("Frame data size does not match frame size.");
    }
    const std::size_t base = frameIndex * frameSize_;
    std::copy(data.begin(), data.end(), memory_.begin() + base);
}

void PhysicalMemory::zeroFrame(std::size_t frameIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Frame index out of range.");
    }
    const std::size_t base = frameIndex * frameSize_;
    std::fill(memory_.begin() + base, memory_.begin() + base + frameSize_, 0);
}

void PhysicalMemory::validateFrameAndOffset(std::size_t frameIndex, std::size_t offset, std::size_t accessSize) const {
    if (frameIndex >= totalFrames_) {
        throw std::out_of_range("Physical frame index out of bounds.");
    }
    if (offset + accessSize > frameSize_) {
        throw std::out_of_range("Physical frame offset out of bounds.");
    }
}
