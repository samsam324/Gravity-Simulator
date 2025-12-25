#include "app_config.h"
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <string>

static int clampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static bool readEnvString(const char* name, std::string& outValue) {
#if defined(_WIN32)
    char* buffer = nullptr;
    size_t length = 0;
    if (_dupenv_s(&buffer, &length, name) != 0 || !buffer) return false;
    outValue.assign(buffer);
    std::free(buffer);
    return !outValue.empty();
#else
    const char* v = std::getenv(name);
    if (!v) return false;
    outValue = v;
    return !outValue.empty();
#endif
}

static int readEnvInt(const char* name, int fallback) {
    std::string value;
    if (!readEnvString(name, value)) return fallback;

    int x = std::atoi(value.c_str());
    return (x > 0) ? x : fallback;
}

static uint32_t readEnvU32(const char* name, uint32_t fallback) {
    std::string value;
    if (!readEnvString(name, value)) return fallback;

    unsigned long long x = std::strtoull(value.c_str(), nullptr, 10);
    if (x == 0ull) return fallback;
    return (uint32_t)x;
}

AppConfig buildAppConfig(const SystemInfo& systemInfo) {
    AppConfig config;

    config.workerThreads = chooseWorkerThreadCount(systemInfo);

    config.particleCount = readEnvInt("GRAVITY_PARTICLES", 25000);
    config.particleCount = clampInt(config.particleCount, 1000, 150000);

    config.deterministicSeed = readEnvU32("GRAVITY_SEED", 13371337u);

    return config;
}
