#include "taskforge/scheduler.hpp"

#include <utility>

namespace taskforge {

Scheduler::Scheduler(std::size_t worker_count, std::size_t queue_capacity)
    : Scheduler(worker_count, queue_capacity, nullptr) {}

Scheduler::Scheduler(
    std::size_t worker_count,
    std::size_t queue_capacity,
    std::shared_ptr<Logger> logger
)
    : dead_letter_store_(std::make_shared<DeadLetterStore>()),
      metrics_(std::make_shared<Metrics>()),
      thread_pool_(worker_count, queue_capacity, dead_letter_store_, std::move(logger), metrics_) {}

bool Scheduler::submit(Job job) {
    return thread_pool_.submit(std::move(job));
}

void Scheduler::shutdown_drain() {
    thread_pool_.shutdown_drain();
}

std::vector<WorkerState> Scheduler::worker_states() const {
    return thread_pool_.worker_states();
}

std::shared_ptr<DeadLetterStore> Scheduler::dead_letter_store() const {
    return dead_letter_store_;
}

std::shared_ptr<Metrics> Scheduler::metrics() const {
    return metrics_;
}

}  