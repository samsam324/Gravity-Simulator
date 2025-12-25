#include "thread_pool.h"
#include <algorithm>

ThreadPool::ThreadPool(unsigned int workerCount)
    : workerTotal(std::max(1u, workerCount)) {
    workers.reserve(workerTotal);
    for (unsigned int i = 0; i < workerTotal; i++) {
        workers.emplace_back([this]() { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopRequested = true;
        jobId++;
    }
    startCondition.notify_all();
    for (auto& t : workers) t.join();
}

unsigned int ThreadPool::workerCount() const {
    return workerTotal;
}

void ThreadPool::parallelFor(std::size_t begin,
                             std::size_t end,
                             std::size_t minGrain,
                             const std::function<void(std::size_t, std::size_t)>& task) {
    if (end <= begin) return;
    std::size_t total = end - begin;
    if (workerTotal == 1 || total <= minGrain * 2) {
        task(begin, end);
        return;
    }

    std::uint64_t thisJob = 0;

    {
        std::lock_guard<std::mutex> lock(mutex);
        currentTask = task;
        nextIndex.store(begin, std::memory_order_relaxed);
        endIndex = end;
        grainSize = std::max<std::size_t>(minGrain, 256);
        remainingWorkers.store(workerTotal, std::memory_order_relaxed);

        jobId++;
        thisJob = jobId;
    }

    startCondition.notify_all();

    std::unique_lock<std::mutex> lock(mutex);
    doneCondition.wait(lock, [&]() { return finishedJobId == thisJob; });
}

void ThreadPool::workerLoop() {
    std::uint64_t lastSeenJob = 0;

    while (true) {
        std::function<void(std::size_t, std::size_t)> task;
        std::size_t localEnd = 0;
        std::size_t localGrain = 0;
        std::uint64_t myJob = 0;

        {
            std::unique_lock<std::mutex> lock(mutex);
            startCondition.wait(lock, [&]() { return stopRequested || jobId != lastSeenJob; });

            if (stopRequested) return;

            lastSeenJob = jobId;
            myJob = jobId;

            task = currentTask;
            localEnd = endIndex;
            localGrain = grainSize;
        }

        while (true) {
            std::size_t start = nextIndex.fetch_add(localGrain, std::memory_order_relaxed);
            if (start >= localEnd) break;
            std::size_t stop = std::min(start + localGrain, localEnd);
            task(start, stop);
        }

        if (remainingWorkers.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            std::lock_guard<std::mutex> lock(mutex);
            finishedJobId = myJob;
            doneCondition.notify_one();
        }
    }
}
