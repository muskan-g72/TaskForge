#include <cassert>

#include "taskforge/job.hpp"
#include "taskforge/retry_policy.hpp"

void test_should_retry_when_under_limit() {
    taskforge::RetryPolicy policy;

    taskforge::Job job;
    job.retry_count = 1;
    job.max_retries = 3;

    assert(policy.should_retry(job));
}

void test_should_not_retry_at_limit() {
    taskforge::RetryPolicy policy;

    taskforge::Job job;
    job.retry_count = 3;
    job.max_retries = 3;

    assert(!policy.should_retry(job));
}

int main() {
    test_should_retry_when_under_limit();
    test_should_not_retry_at_limit();

    return 0;
}