#include "taskforge/retry_policy.hpp"

namespace taskforge {

bool RetryPolicy::should_retry(const Job& job) const {
    return job.retry_count < job.max_retries;
}

}  