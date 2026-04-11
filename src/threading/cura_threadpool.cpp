/**
 * @file cura_threadpool.cpp
 * @brief Thread pool implementation
 */

#include "cura_threadpool.hpp"

namespace cura {

CuraThreadPool::CuraThreadPool(size_t num_threads)
    : stop_(false)
    , paused_(false)
    , active_tasks_(0) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

CuraThreadPool::~CuraThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }

    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void CuraThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            condition_.wait(lock, [this] {
                return stop_ || (!paused_ && !tasks_.empty());
            });

            if (stop_ && tasks_.empty()) {
                return;
            }

            if (paused_) {
                continue;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        active_tasks_++;
        task();
        active_tasks_--;

        completed_.notify_all();
    }
}

void CuraThreadPool::submit_batch(std::vector<std::function<void()>>& tasks) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        for (auto& task : tasks) {
            tasks_.push(std::move(task));
        }
    }

    condition_.notify_all();
}

void CuraThreadPool::wait_all() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    completed_.wait(lock, [this] {
        return tasks_.empty() && active_tasks_ == 0;
    });
}

size_t CuraThreadPool::size() const {
    return workers_.size();
}

size_t CuraThreadPool::pending_tasks() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queue_mutex_));
    return tasks_.size() + active_tasks_;
}

void CuraThreadPool::pause() {
    paused_ = true;
}

void CuraThreadPool::resume() {
    paused_ = false;
    condition_.notify_all();
}

bool CuraThreadPool::is_paused() const {
    return paused_;
}

} // namespace cura