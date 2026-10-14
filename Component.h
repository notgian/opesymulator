#pragma once

#include <string>
#include <vector>

class Component {
    public:
        Component(int row, int col, int width, int height);
        virtual ~Component() = default;

        int getX() const;
        int getY() const;
        int getWidth() const;
        int getHeight() const;
        std::vector<std::string> getText() const;
        
        void setX(int x);
        void setY(int y);
        void setWidth(int width);
        void setHeight(int height);
        void setText(const std::vector<std::string>& text);
        void setText(const std::string& text);

    protected:
        int x_;
        int y_;
        int width_;
        int height_;

        std::vector<std::string> text_ ;
};

