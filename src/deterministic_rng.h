#pragma once
#include <cstdint>

struct DeterministicRng {
    uint32_t state;
    explicit DeterministicRng(uint32_t seed = 0x12345678u) : state(seed ? seed : 0x12345678u) {}
    uint32_t nextU32() {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state = x;
        return x;
    }
    float nextFloat01() {
        return (nextU32() >> 8) * (1.0f / 16777216.0f);
    }
    float range(float a, float b) {
        return a + (b - a) * nextFloat01();
    }
};
