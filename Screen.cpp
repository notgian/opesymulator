#include "Screen.h"
#include <iostream>

Screen::Screen(int width, int height) 
    : width_(width), height_(height) {
    clearBuffer();
}

int Screen::getWidth() const { return width_; }
int Screen::getHeight() const { return height_; }

void Screen::resize(int width, int height) {
    width_ = width;
    height_ = height;
    // clearBuffer();
}

void Screen::clearBuffer() {
    screenBuffer_.assign(height_, std::string(width_, ' '));
}

void Screen::update(const Component& component) {
    int compX = component.getX();
    int compY = component.getY();
    const std::vector<std::string>& text = component.getText();

    for (int r = 0; r < text.size(); ++r) {
        int screenY = compY + r;
        // Skip rows that fall outside the screen
        if (screenY < 0 || screenY >= height_) continue;

        const std::string& line = text[r];
        for (int c = 0; c < line.size(); ++c) {
            int screenX = compX + c;

            // Skip columns that fall outside the screen horizontally
            if (screenX < 0 || screenX >= width_) continue;

            // Composite character onto screen buffer
            screenBuffer_[screenY][screenX] = line[c];
        }
    }
}

// Does not update the buffer; Only draws to stdout
void Screen::draw() const {
    std::string renderedDisplay;
    
    // Optional performance boost: reserve memory up-front if screenBuffer_ is not empty
    if (!screenBuffer_.empty()) {
        renderedDisplay.reserve(screenBuffer_.size() * (screenBuffer_[0].size() + 10));
    }
    
    renderedDisplay += "\033[H";

    for (const auto& row : screenBuffer_) {
        // Appends the row content, clears residual characters to the end of the line, and adds a newline
        renderedDisplay += row;
        renderedDisplay += "\033[K\n";
    }

    std::cout << renderedDisplay << std::flush;
}
