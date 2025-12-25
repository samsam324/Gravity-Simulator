#include "system_info.h"
#include <thread>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#if defined(_MSC_VER)
  #include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
  #include <cpuid.h>
#endif

#include <SFML/OpenGL.hpp>

static std::string cpuBrand() {
#if defined(_MSC_VER)
    int cpuInfo[4] = {0};
    char brand[49] = {0};
    __cpuid(cpuInfo, 0x80000000);
    unsigned int maxExt = (unsigned int)cpuInfo[0];
    if (maxExt < 0x80000004) return "Unknown CPU";
    __cpuid(cpuInfo, 0x80000002); std::memcpy(brand + 0,  cpuInfo, 16);
    __cpuid(cpuInfo, 0x80000003); std::memcpy(brand + 16, cpuInfo, 16);
    __cpuid(cpuInfo, 0x80000004); std::memcpy(brand + 32, cpuInfo, 16);
    return std::string(brand);
#elif defined(__GNUC__) || defined(__clang__)
    unsigned int maxExt = __get_cpuid_max(0x80000000, nullptr);
    if (maxExt < 0x80000004) return "Unknown CPU";
    char brand[49] = {0};
    unsigned int a, b, c, d;
    __get_cpuid(0x80000002, &a, &b, &c, &d); std::memcpy(brand + 0,  &a, 4); std::memcpy(brand + 4,  &b, 4); std::memcpy(brand + 8,  &c, 4); std::memcpy(brand + 12, &d, 4);
    __get_cpuid(0x80000003, &a, &b, &c, &d); std::memcpy(brand + 16, &a, 4); std::memcpy(brand + 20, &b, 4); std::memcpy(brand + 24, &c, 4); std::memcpy(brand + 28, &d, 4);
    __get_cpuid(0x80000004, &a, &b, &c, &d); std::memcpy(brand + 32, &a, 4); std::memcpy(brand + 36, &b, 4); std::memcpy(brand + 40, &c, 4); std::memcpy(brand + 44, &d, 4);
    return std::string(brand);
#else
    return "Unknown CPU";
#endif
}

static std::string glStringOrUnknown(GLenum name) {
    const GLubyte* value = glGetString(name);
    if (!value) return "Unknown";
    return reinterpret_cast<const char*>(value);
}

SystemInfo detectSystemInfo() {
    SystemInfo info;
    unsigned int hc = std::thread::hardware_concurrency();
    info.detectedHardwareThreads = hc ? hc : 1;
    info.cpuBrandString = cpuBrand();
    info.gpuVendorString = glStringOrUnknown(GL_VENDOR);
    info.gpuRendererString = glStringOrUnknown(GL_RENDERER);
    return info;
}

unsigned int chooseWorkerThreadCount(const SystemInfo& info) {
    unsigned int maxThreads = info.detectedHardwareThreads;
    const char* overrideEnv = std::getenv("GRAVITY_THREADS");
    if (overrideEnv) {
        int value = std::atoi(overrideEnv);
        if (value > 0) return (unsigned int)value;
    }
    if (maxThreads <= 2) return 1;
    return std::max(1u, maxThreads - 1u);
}
