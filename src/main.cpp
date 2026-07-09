#include <atomic>
#include <iostream>
#include <stdexcept>

#include "taskforge/job.hpp"
#include "taskforge/scheduler.hpp"

int main() {
    taskforge::Scheduler scheduler(4, 10);

    std::atomic<int> completed_jobs{0};
    std::atomic<int> attempts_for_failing_job{0};

    for (std::uint64_t id = 1; id <= 5; ++id) {
        taskforge::Job job;
        job.job_id = id;
        job.task = [&completed_jobs, id] {
            ++completed_jobs;
            std::cout << "completed job " << id << '\n';
        };

        scheduler.submit(job);
    }

    taskforge::Job eventually_successful_job;
    eventually_successful_job.job_id = 100;
    eventually_successful_job.max_retries = 3;
    eventually_successful_job.task = [&] {
        int attempt = ++attempts_for_failing_job;

        if (attempt < 3) {
            throw std::runtime_error("temporary failure");
        }

        ++completed_jobs;
        std::cout << "job 100 succeeded after retries\n";
    };

    scheduler.submit(eventually_successful_job);

    taskforge::Job permanently_failing_job;
    permanently_failing_job.job_id = 200;
    permanently_failing_job.max_retries = 2;
    permanently_failing_job.task = [] {
        throw std::runtime_error("permanent failure");
    };

    scheduler.submit(permanently_failing_job);

    scheduler.shutdown_drain();

    std::cout << "total completed jobs: " << completed_jobs.load() << '\n';
    std::cout << "dead-letter jobs: " << scheduler.dead_letter_store()->size() << '\n';
    std::cout << "scheduler shutdown complete\n";

    return 0;
}