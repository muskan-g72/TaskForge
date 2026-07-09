#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

#include "taskforge/job.hpp"

namespace taskforge {

class DeadLetterStore {
public:
    void add(Job job);

    std::vector<Job> jobs() const;
    std::size_t size() const;
    bool empty() const;

private:
    mutable std::mutex mutex_;
    std::vector<Job> jobs_;
};

}  