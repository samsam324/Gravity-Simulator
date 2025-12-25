#include "particles.h"

void Particles::reserve(std::size_t count) {
    positionX.reserve(count);
    positionY.reserve(count);
    velocityX.reserve(count);
    velocityY.reserve(count);
    mass.reserve(count);
}

void Particles::clear() {
    positionX.clear();
    positionY.clear();
    velocityX.clear();
    velocityY.clear();
    mass.clear();
}

void Particles::add(float x, float y, float vx, float vy, float m) {
    positionX.push_back(x);
    positionY.push_back(y);
    velocityX.push_back(vx);
    velocityY.push_back(vy);
    mass.push_back(m);
}

std::size_t Particles::count() const {
    return positionX.size();
}
