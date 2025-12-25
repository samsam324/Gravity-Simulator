#pragma once
#include <vector>
#include <cstddef>

struct Particles {
    std::vector<float> positionX;
    std::vector<float> positionY;
    std::vector<float> velocityX;
    std::vector<float> velocityY;
    std::vector<float> mass;

    void reserve(std::size_t count);
    void clear();
    void add(float x, float y, float vx, float vy, float m);
    std::size_t count() const;
};
