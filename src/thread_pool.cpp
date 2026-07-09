#include "taskforge/thread_pool.hpp"

#include <chrono>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace taskforge {

ThreadPool::ThreadPool(std::size_t worker_count, std::size_t queue_capacity)
    : ThreadPool(
          worker_count,
          queue_capacity,
          std::make_shared<DeadLetterStore>(),
          nullptr,
          std::make_shared<Metrics>()
      ) {}

ThreadPool::ThreadPool(
    std::size_t worker_count,
    std::size_t queue_capacity,
    std::shared_ptr<DeadLetterStore> dead_letter_store
)
    : ThreadPool(
          worker_count,
          queue_capacity,
          std::move(dead_letter_store),
          nullptr,
          std::make_shared<Metrics>()
      ) {}

ThreadPool::ThreadPool(
    std::size_t worker_count,
    std::size_t queue_capacity,
    std::shared_ptr<DeadLetterStore> dead_letter_store,
    std::shared_ptr<Logger> logger
)
    : ThreadPool(
          worker_count,
          queue_capacity,
          std::move(dead_letter_store),
          std::move(logger),
          std::make_shared<Metrics>()
      ) {}

ThreadPool::ThreadPool(
    std::size_t worker_count,
    std::size_t queue_capacity,
    std::shared_ptr<DeadLetterStore> dead_letter_store,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<Metrics> metrics
)
    : queue_(queue_capacity),
      worker_states_(worker_count, WorkerState::Starting),
      dead_letter_store_(std::move(dead_letter_store)),
      logger_(std::move(logger)),
      metrics_(std::move(metrics)) {
    if (worker_count == 0) {
        throw std::invalid_argument("ThreadPool worker_count must be greater than zero");
    }

    if (!dead_letter_store_) {
        throw std::invalid_argument("ThreadPool dead_letter_store must not be null");
    }

    if (!metrics_) {
        throw std::invalid_argument("ThreadPool metrics must not be null");
    }

    workers_.reserve(worker_count);

    for (std::size_t worker_id = 0; worker_id < worker_count; ++worker_id) {
        workers_.emplace_back(&ThreadPool::worker_loop, this, worker_id);
    }
}

ThreadPool::~ThreadPool() {
    shutdown_drain();
}

bool ThreadPool::submit(Job job) {
    {
        std::lock_guard<std::mutex> lock(lifecycle_mutex_);

        if (!accepting_jobs_) {
            return false;
        }

        job.status = JobStatus::Queued;
        ++outstanding_jobs_;
    }

    if (logger_) {
        logger_->job_queued(job.job_id, job.retry_count);
    }

    if (metrics_) {
        metrics_->record_submitted();
    }

    if (!queue_.push(std::move(job))) {
        mark_job_finished();
        return false;
    }

    return true;
}

void ThreadPool::shutdown_drain() {
    {
        std::unique_lock<std::mutex> lock(lifecycle_mutex_);
        accepting_jobs_ = false;

        lifecycle_cv_.wait(lock, [this] {
            return outstanding_jobs_ == 0;
        });

        queue_.shutdown();
    }

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

std::vector<WorkerState> ThreadPool::worker_states() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return worker_states_;
}

std::shared_ptr<DeadLetterStore> ThreadPool::dead_letter_store() const {
    return dead_letter_store_;
}

std::shared_ptr<Metrics> ThreadPool::metrics() const {
    return metrics_;
}

void ThreadPool::worker_loop(std::size_t worker_id) {
    set_worker_state(worker_id, WorkerState::Idle);

    Job job;

    while (queue_.pop(job)) {
        set_worker_state(worker_id, WorkerState::Running);
        execute_job(std::move(job), worker_id);
        set_worker_state(worker_id, WorkerState::Idle);
    }

    set_worker_state(worker_id, WorkerState::Stopping);
    set_worker_state(worker_id, WorkerState::Stopped);
}

void ThreadPool::execute_job(Job job, std::size_t worker_id) {
    job.worker_id = static_cast<int>(worker_id);
    job.status = JobStatus::Running;
    job.started_at = std::chrono::steady_clock::now();

    if (logger_) {
        logger_->job_started(job.job_id, job.worker_id, job.retry_count);
    }

    try {
        if (job.task) {
            job.task();
        }

        job.completed_at = std::chrono::steady_clock::now();

        if (has_timed_out(job)) {
            job.status = JobStatus::TimedOut;

            std::ostringstream message;
            message << "job timed out after " << job.timeout_ms << " ms";
            job.failure_reason = message.str();

            if (logger_) {
                logger_->job_timed_out(job.job_id, job.worker_id, job.failure_reason);
            }

            if (metrics_) {
                metrics_->record_timed_out();
            }

            handle_failed_job(std::move(job));
            return;
        }

        job.status = JobStatus::Completed;

        if (logger_) {
            logger_->job_completed(job.job_id, job.worker_id);
        }

        if (metrics_) {
            metrics_->record_completed(job_latency(job));
        }

        mark_job_finished();
    } catch (const std::exception& error) {
        job.status = JobStatus::Failed;
        job.failure_reason = error.what();
        job.completed_at = std::chrono::steady_clock::now();

        if (logger_) {
            logger_->job_failed(job.job_id, job.worker_id, job.failure_reason);
        }

        if (metrics_) {
            metrics_->record_failed();
        }

        handle_failed_job(std::move(job));
    } catch (...) {
        job.status = JobStatus::Failed;
        job.failure_reason = "unknown exception";
        job.completed_at = std::chrono::steady_clock::now();

        if (logger_) {
            logger_->job_failed(job.job_id, job.worker_id, job.failure_reason);
        }

        if (metrics_) {
            metrics_->record_failed();
        }

        handle_failed_job(std::move(job));
    }
}

void ThreadPool::handle_failed_job(Job job) {
    if (retry_policy_.should_retry(job)) {
        ++job.retry_count;
        job.status = JobStatus::Queued;

        if (logger_) {
            logger_->job_retried(job.job_id, job.retry_count);
        }

        if (metrics_) {
            metrics_->record_retried();
        }

        if (queue_.try_push(job)) {
            return;
        }

        job.status = JobStatus::Failed;
        job.failure_reason = "retry queue full";
    }

    if (logger_) {
        logger_->job_dead_lettered(job.job_id, job.failure_reason);
    }

    if (metrics_) {
        metrics_->record_dead_lettered(job_latency(job));
    }

    dead_letter_store_->add(std::move(job));
    mark_job_finished();
}

bool ThreadPool::has_timed_out(const Job& job) const {
    if (job.timeout_ms <= 0) {
        return false;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        job.completed_at - job.started_at
    );

    return elapsed.count() > job.timeout_ms;
}

std::chrono::milliseconds ThreadPool::job_latency(const Job& job) const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        job.completed_at - job.created_at
    );
}

void ThreadPool::mark_job_finished() {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);

    if (outstanding_jobs_ > 0) {
        --outstanding_jobs_;
    }

    if (outstanding_jobs_ == 0) {
        lifecycle_cv_.notify_all();
    }
}

void ThreadPool::set_worker_state(std::size_t worker_id, WorkerState state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    worker_states_[worker_id] = state;
}

}  