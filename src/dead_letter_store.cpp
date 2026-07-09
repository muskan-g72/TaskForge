#include "taskforge/dead_letter_store.hpp"

#include <utility>

namespace taskforge {

void DeadLetterStore::add(Job job) {
    job.status = JobStatus::DeadLettered;

    std::lock_guard<std::mutex> lock(mutex_);
    jobs_.push_back(std::move(job));
}

std::vector<Job> DeadLetterStore::jobs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_;
}

std::size_t DeadLetterStore::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_.size();
}

bool DeadLetterStore::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_.empty();
}

}  