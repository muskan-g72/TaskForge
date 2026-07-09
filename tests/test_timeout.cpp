#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>

#include "taskforge/job.hpp"
#include "taskforge/scheduler.hpp"

void test_fast_job_does_not_timeout() {
    taskforge::Scheduler scheduler(2, 10);

    std::atomic<int> completed{0};

    taskforge::Job job;
    job.job_id = 1;
    job.timeout_ms = 100;
    job.task = [&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++completed;
    };

    assert(scheduler.submit(job));

    scheduler.shutdown_drain();

    assert(completed.load() == 1);
    assert(scheduler.dead_letter_store()->empty());
}

void test_slow_job_retries_then_goes_to_dead_letter() {
    taskforge::Scheduler scheduler(2, 10);

    std::atomic<int> attempts{0};

    taskforge::Job job;
    job.job_id = 2;
    job.timeout_ms = 20;
    job.max_retries = 2;
    job.task = [&] {
        ++attempts;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    };

    assert(scheduler.submit(job));

    scheduler.shutdown_drain();

    assert(attempts.load() == 3);
    assert(scheduler.dead_letter_store()->size() == 1);

    auto jobs = scheduler.dead_letter_store()->jobs();

    assert(jobs.size() == 1);
    assert(jobs[0].job_id == 2);
    assert(jobs[0].status == taskforge::JobStatus::DeadLettered);
    assert(jobs[0].retry_count == 2);
    assert(jobs[0].failure_reason == "job timed out after 20 ms");
}

int main() {
    test_fast_job_does_not_timeout();
    test_slow_job_retries_then_goes_to_dead_letter();

    return 0;
}