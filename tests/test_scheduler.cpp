#include <atomic>
#include <cassert>

#include "taskforge/job.hpp"
#include "taskforge/scheduler.hpp"

void test_scheduler_submits_and_completes_jobs() {
    taskforge::Scheduler scheduler(3, 10);

    std::atomic<int> completed{0};

    for (int i = 0; i < 25; ++i) {
        taskforge::Job job;
        job.job_id = static_cast<std::uint64_t>(i + 1);
        job.task = [&] {
            ++completed;
        };

        assert(scheduler.submit(job));
    }

    scheduler.shutdown_drain();

    assert(completed.load() == 25);
}

void test_scheduler_rejects_after_shutdown() {
    taskforge::Scheduler scheduler(2, 4);

    scheduler.shutdown_drain();

    taskforge::Job job;
    job.job_id = 1;
    job.task = [] {};

    assert(!scheduler.submit(job));
}

int main() {
    test_scheduler_submits_and_completes_jobs();
    test_scheduler_rejects_after_shutdown();

    return 0;
}