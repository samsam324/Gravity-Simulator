#pragma once
#include <cstdint>
#include "system_info.h"

struct AppConfig {
    uint32_t deterministicSeed = 13371337u;
    int particleCount = 25000;
    unsigned int workerThreads = 1;
};

AppConfig buildAppConfig(const SystemInfo& systemInfo);
