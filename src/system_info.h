#pragma once
#include <string>

struct SystemInfo {
    unsigned int detectedHardwareThreads = 1;
    std::string cpuBrandString;
    std::string gpuVendorString;
    std::string gpuRendererString;
};

SystemInfo detectSystemInfo();
unsigned int chooseWorkerThreadCount(const SystemInfo& info);
