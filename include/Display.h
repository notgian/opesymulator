#pragma once

#include "Component.h"
#include <vector>
#include <string>

class Display {
    public:
        Display(int width, int height);
        virtual ~Display() = default;

        // --- Display Dimensions ---
        int getWidth() const;
        int getHeight() const;
        void resize(int width, int height);

        // --- Display & Rendering ---
        // Composites a single components onto the display buffer
        void update(const Component &component);

        // Prints buffer to stdout
        void draw() const;

        // Clears the master display buffer to spaces
        void clearBuffer();

    protected:
        int width_;
        int height_;

        std::vector<std::string> screenBuffer_;
};
