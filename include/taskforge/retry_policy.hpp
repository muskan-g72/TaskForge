#pragma once

#include "taskforge/job.hpp"

namespace taskforge {

class RetryPolicy {
public:
    bool should_retry(const Job& job) const;
};

}  