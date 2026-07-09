#include <atomic>
#include <cassert>
#include <stdexcept>

#include "taskforge/job.hpp"
#include "taskforge/scheduler.hpp"

void test_job_succeeds_after_retry() {
    taskforge::Scheduler scheduler(2, 10);

    std::atomic<int> attempts{0};
    std::atomic<int> completed{0};

    taskforge::Job job;
    job.job_id = 100;
    job.max_retries = 3;
    job.task = [&] {
        int attempt = ++attempts;

        if (attempt < 3) {
            throw std::runtime_error("temporary failure");
        }

        ++completed;
    };

    assert(scheduler.submit(job));

    scheduler.shutdown_drain();

    assert(attempts.load() == 3);
    assert(completed.load() == 1);
    assert(scheduler.dead_letter_store()->empty());
}

void test_job_goes_to_dead_letter_after_retry_exhaustion() {
    taskforge::Scheduler scheduler(2, 10);

    std::atomic<int> attempts{0};

    taskforge::Job job;
    job.job_id = 200;
    job.max_retries = 2;
    job.task = [&] {
        ++attempts;
        throw std::runtime_error("permanent failure");
    };

    assert(scheduler.submit(job));

    scheduler.shutdown_drain();

    assert(attempts.load() == 3);
    assert(scheduler.dead_letter_store()->size() == 1);

    auto dead_jobs = scheduler.dead_letter_store()->jobs();

    assert(dead_jobs[0].job_id == 200);
    assert(dead_jobs[0].status == taskforge::JobStatus::DeadLettered);
    assert(dead_jobs[0].retry_count == 2);
    assert(dead_jobs[0].failure_reason == "permanent failure");
}

int main() {
    test_job_succeeds_after_retry();
    test_job_goes_to_dead_letter_after_retry_exhaustion();

    return 0;
}