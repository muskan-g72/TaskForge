#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "taskforge/dead_letter_store.hpp"
#include "taskforge/job.hpp"
#include "taskforge/logger.hpp"
#include "taskforge/metrics.hpp"
#include "taskforge/thread_pool.hpp"
#include "taskforge/worker_state.hpp"

namespace taskforge {

class Scheduler {
public:
    Scheduler(std::size_t worker_count, std::size_t queue_capacity);
    Scheduler(std::size_t worker_count, std::size_t queue_capacity, std::shared_ptr<Logger> logger);

    bool submit(Job job);
    void shutdown_drain();

    std::vector<WorkerState> worker_states() const;
    std::shared_ptr<DeadLetterStore> dead_letter_store() const;
    std::shared_ptr<Metrics> metrics() const;

private:
    std::shared_ptr<DeadLetterStore> dead_letter_store_;
    std::shared_ptr<Metrics> metrics_;
    ThreadPool thread_pool_;
};

}  