#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstddef>
#include <cstdint>

class ThreadPool {
public:
    explicit ThreadPool(unsigned int workerCount);
    ~ThreadPool();

    unsigned int workerCount() const;

    void parallelFor(std::size_t begin,
                     std::size_t end,
                     std::size_t minGrain,
                     const std::function<void(std::size_t, std::size_t)>& task);

private:
    void workerLoop();

    unsigned int workerTotal = 1;
    std::vector<std::thread> workers;

    std::mutex mutex;
    std::condition_variable startCondition;
    std::condition_variable doneCondition;

    std::function<void(std::size_t, std::size_t)> currentTask;

    std::atomic<std::size_t> nextIndex{0};
    std::size_t endIndex = 0;
    std::size_t grainSize = 1024;

    std::atomic<unsigned int> remainingWorkers{0};

    bool stopRequested = false;

    std::uint64_t jobId = 0;
    std::uint64_t finishedJobId = 0;
};
