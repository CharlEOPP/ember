#pragma once
#include "ember/core/Types.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace ember {

// Minimal fixed-size worker pool for background asset I/O / CPU decode. GL work
// never runs here — loaders hand the GPU step back to the main thread via the
// AssetManager's PendingUpload queue.
class ThreadPool {
public:
    explicit ThreadPool(usize threads = 0) {
        if (threads == 0) {
            const unsigned hc = std::thread::hardware_concurrency();
            threads = hc > 1 ? static_cast<usize>(hc - 1) : 1;
        }
        for (usize i = 0; i < threads; ++i)
            m_workers.emplace_back([this] { workerLoop(); });
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_stopping = true;
        }
        m_cv.notify_all();
        for (auto& w : m_workers) if (w.joinable()) w.join();
    }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void enqueue(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_jobs.push(std::move(job));
        }
        m_cv.notify_one();
    }

    usize size() const { return m_workers.size(); }

private:
    void workerLoop() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lk(m_mutex);
                m_cv.wait(lk, [this] { return m_stopping || !m_jobs.empty(); });
                if (m_stopping && m_jobs.empty()) return;
                job = std::move(m_jobs.front());
                m_jobs.pop();
            }
            job();
        }
    }

    std::vector<std::thread>          m_workers;
    std::queue<std::function<void()>> m_jobs;
    std::mutex                        m_mutex;
    std::condition_variable           m_cv;
    bool                              m_stopping = false;
};

} // namespace ember
