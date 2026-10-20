#include "Component.h"
#include <sstream>

Component::Component(int row, int col, int width, int height)
    : x_(col), y_(row), width_(width), height_(height) {
        // Pre-allocate the text vector to match height_ with empty lines
        text_.resize(height_, std::string(width_, ' '));
    }

// --- Getters ---

int Component::getX() const {
    return x_;
}

int Component::getY() const {
    return y_;
}

int Component::getWidth() const {
    return width_;
}

int Component::getHeight() const {
    return height_;
}

std::vector<std::string> Component::getText() const {
    std::lock_guard<std::mutex> lock(textMutex_);
    return text_;
}

// --- Setters ---

void Component::setX(int x) {
    x_ = x;
}

void Component::setY(int y) {
    y_ = y;
}

void Component::setWidth(int width) {
    std::lock_guard<std::mutex> lock(textMutex_);
    if (width == width_) return;

    width_ = width;

    for (std::string& line : text_) {
        line.resize(width_, ' ');
    }
}

void Component::setHeight(int height) {
    std::lock_guard<std::mutex> lock(textMutex_);
    if (height == height_) return;

    height_ = height;

    if (height_ > text_.size()) {
        // Grow: append empty padded strings to fill missing rows
        text_.resize(height_, std::string(width_, ' '));
    } else {
        // Shrink: keep the top rows, drop the bottom ones
        text_.resize(height_);
    }
}

void Component::setText(const std::vector<std::string>& text) {
    std::lock_guard<std::mutex> lock(textMutex_);
    text_ = text;
}

// Convenience overload: splits string by newline character ('\n')
void Component::setText(const std::string& text) {
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);

    std::lock_guard<std::mutex> lock(textMutex_);
    text_ = std::move(lines);
}
