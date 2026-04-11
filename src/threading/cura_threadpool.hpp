/**
 * @file cura_threadpool.hpp
 * @brief Thread pool for parallel processing
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace cura {

/**
 * @brief Task priority levels
 */
enum class TaskPriority {
    HIGH,    // UI tasks
    NORMAL,  // Hash tasks
    LOW      // Thumbnail generation
};

/**
 * @brief Thread pool with work-stealing scheduler
 */
class CuraThreadPool {
public:
    /**
     * @brief Construct thread pool with specified number of threads
     * @param num_threads Number of worker threads (default: hardware concurrency)
     */
    explicit CuraThreadPool(size_t num_threads = std::thread::hardware_concurrency());

    ~CuraThreadPool();

    /**
     * @brief Submit a task to the pool
     * @param task Task function to execute
     * @param priority Task priority (default: NORMAL)
     * @return Future for the task result
     */
    template<typename F>
    auto submit(F task, TaskPriority priority = TaskPriority::NORMAL) -> std::future<decltype(task())>;

    /**
     * @brief Submit multiple tasks and wait for all to complete
     * @param tasks Vector of task functions
     */
    void submit_batch(std::vector<std::function<void()>>& tasks);

    /**
     * @brief Wait for all pending tasks to complete
     */
    void wait_all();

    /**
     * @brief Get number of worker threads
     */
    size_t size() const;

    /**
     * @brief Get number of pending tasks
     */
    size_t pending_tasks() const;

    /**
     * @brief Pause execution of tasks
     */
    void pause();

    /**
     * @brief Resume execution of tasks
     */
    void resume();

    /**
     * @brief Check if pool is paused
     */
    bool is_paused() const;

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable completed_;
    std::atomic<bool> stop_;
    std::atomic<bool> paused_;
    std::atomic<size_t> active_tasks_;

    void worker_loop();
};

// Template implementation
template<typename F>
auto CuraThreadPool::submit(F task, TaskPriority priority) -> std::future<decltype(task())> {
    using ReturnType = decltype(task());

    auto packaged_task = std::make_shared<std::packaged_task<ReturnType()>>(std::move(task));
    auto future = packaged_task->get_future();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        tasks_.emplace([packaged_task]() { (*packaged_task)(); });
    }

    condition_.notify_one();
    return future;
}

} // namespace cura