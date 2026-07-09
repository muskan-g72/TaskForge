#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <thread>

#include "taskforge/job.hpp"
#include "taskforge/thread_pool.hpp"

void test_single_job_completes() {
    taskforge::ThreadPool pool(2, 4);

    std::atomic<int> completed{0};

    taskforge::Job job;
    job.job_id = 1;
    job.task = [&] {
        ++completed;
    };

    assert(pool.submit(job));

    pool.shutdown_drain();

    assert(completed.load() == 1);
}

void test_many_jobs_complete() {
    taskforge::ThreadPool pool(4, 20);

    std::atomic<int> completed{0};

    for (int i = 0; i < 100; ++i) {
        taskforge::Job job;
        job.job_id = static_cast<std::uint64_t>(i + 1);
        job.task = [&] {
            ++completed;
        };

        assert(pool.submit(job));
    }

    pool.shutdown_drain();

    assert(completed.load() == 100);
}

void test_multiple_workers_run_concurrently() {
    taskforge::ThreadPool pool(4, 8);

    std::atomic<int> running{0};
    std::atomic<int> max_running{0};
    std::atomic<int> completed{0};

    for (int i = 0; i < 8; ++i) {
        taskforge::Job job;
        job.job_id = static_cast<std::uint64_t>(i + 1);

        job.task = [&] {
            int current_running = ++running;

            int observed = max_running.load();
            while (current_running > observed &&
                   !max_running.compare_exchange_weak(observed, current_running)) {
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            --running;
            ++completed;
        };

        assert(pool.submit(job));
    }

    pool.shutdown_drain();

    assert(completed.load() == 8);
    assert(max_running.load() >= 2);
}

void test_submit_fails_after_shutdown() {
    taskforge::ThreadPool pool(2, 4);

    pool.shutdown_drain();

    taskforge::Job job;
    job.job_id = 1;
    job.task = [] {};

    assert(!pool.submit(job));
}

void test_failed_job_goes_to_dead_letter_store() {
    taskforge::ThreadPool pool(2, 4);

    taskforge::Job job;
    job.job_id = 99;
    job.max_retries = 0;
    job.task = [] {
        throw std::runtime_error("always fails");
    };

    assert(pool.submit(job));

    pool.shutdown_drain();

    auto store = pool.dead_letter_store();

    assert(store->size() == 1);

    auto jobs = store->jobs();

    assert(jobs.size() == 1);
    assert(jobs[0].job_id == 99);
    assert(jobs[0].status == taskforge::JobStatus::DeadLettered);
    assert(jobs[0].failure_reason == "always fails");
}

int main() {
    test_single_job_completes();
    test_many_jobs_complete();
    test_multiple_workers_run_concurrently();
    test_submit_fails_after_shutdown();
    test_failed_job_goes_to_dead_letter_store();

    return 0;
}