#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "taskforge/bounded_queue.hpp"
#include "taskforge/dead_letter_store.hpp"
#include "taskforge/job.hpp"
#include "taskforge/logger.hpp"
#include "taskforge/metrics.hpp"
#include "taskforge/retry_policy.hpp"
#include "taskforge/worker_state.hpp"

namespace taskforge {

class ThreadPool {
public:
    ThreadPool(std::size_t worker_count, std::size_t queue_capacity);
    ThreadPool(
        std::size_t worker_count,
        std::size_t queue_capacity,
        std::shared_ptr<DeadLetterStore> dead_letter_store
    );
    ThreadPool(
        std::size_t worker_count,
        std::size_t queue_capacity,
        std::shared_ptr<DeadLetterStore> dead_letter_store,
        std::shared_ptr<Logger> logger
    );
    ThreadPool(
        std::size_t worker_count,
        std::size_t queue_capacity,
        std::shared_ptr<DeadLetterStore> dead_letter_store,
        std::shared_ptr<Logger> logger,
        std::shared_ptr<Metrics> metrics
    );

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    bool submit(Job job);
    void shutdown_drain();

    std::vector<WorkerState> worker_states() const;
    std::shared_ptr<DeadLetterStore> dead_letter_store() const;
    std::shared_ptr<Metrics> metrics() const;

private:
    void worker_loop(std::size_t worker_id);
    void execute_job(Job job, std::size_t worker_id);
    void handle_failed_job(Job job);
    bool has_timed_out(const Job& job) const;
    std::chrono::milliseconds job_latency(const Job& job) const;
    void mark_job_finished();
    void set_worker_state(std::size_t worker_id, WorkerState state);

    BoundedQueue<Job> queue_;
    std::vector<std::thread> workers_;

    mutable std::mutex state_mutex_;
    std::vector<WorkerState> worker_states_;

    bool accepting_jobs_ = true;
    std::size_t outstanding_jobs_ = 0;
    mutable std::mutex lifecycle_mutex_;
    std::condition_variable lifecycle_cv_;

    RetryPolicy retry_policy_;
    std::shared_ptr<DeadLetterStore> dead_letter_store_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<Metrics> metrics_;
};

}  