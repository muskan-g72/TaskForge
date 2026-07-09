#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "taskforge/job.hpp"
#include "taskforge/scheduler.hpp"

void test_scheduler_metrics_for_successful_jobs() {
    taskforge::Scheduler scheduler(2, 10);

    for (int i = 0; i < 5; ++i) {
        taskforge::Job job;
        job.job_id = static_cast<std::uint64_t>(i + 1);
        job.task = [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        };

        assert(scheduler.submit(job));
    }

    scheduler.shutdown_drain();

    auto snapshot = scheduler.metrics()->snapshot();

    assert(snapshot.submitted_jobs == 5);
    assert(snapshot.completed_jobs == 5);
    assert(snapshot.dead_lettered_jobs == 0);
    assert(snapshot.average_latency_ms >= 0.0);
    assert(snapshot.throughput_jobs_per_sec > 0.0);
}

void test_scheduler_metrics_for_failure_and_dead_letter() {
    taskforge::Scheduler scheduler(2, 10);

    taskforge::Job job;
    job.job_id = 10;
    job.max_retries = 1;
    job.task = [] {
        throw std::runtime_error("fail");
    };

    assert(scheduler.submit(job));

    scheduler.shutdown_drain();

    auto snapshot = scheduler.metrics()->snapshot();

    assert(snapshot.submitted_jobs == 1);
    assert(snapshot.failed_jobs == 2);
    assert(snapshot.retried_jobs == 1);
    assert(snapshot.dead_lettered_jobs == 1);
}

void test_scheduler_metrics_for_timeout() {
    taskforge::Scheduler scheduler(2, 10);

    taskforge::Job job;
    job.job_id = 20;
    job.timeout_ms = 5;
    job.max_retries = 0;
    job.task = [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    };

    assert(scheduler.submit(job));

    scheduler.shutdown_drain();

    auto snapshot = scheduler.metrics()->snapshot();

    assert(snapshot.submitted_jobs == 1);
    assert(snapshot.timed_out_jobs == 1);
    assert(snapshot.dead_lettered_jobs == 1);
}

int main() {
    test_scheduler_metrics_for_successful_jobs();
    test_scheduler_metrics_for_failure_and_dead_letter();
    test_scheduler_metrics_for_timeout();

    return 0;
}