#include <cassert>
#include <string>

#include "taskforge/dead_letter_store.hpp"
#include "taskforge/job.hpp"

void test_dead_letter_store_adds_job() {
    taskforge::DeadLetterStore store;

    taskforge::Job job;
    job.job_id = 42;
    job.status = taskforge::JobStatus::Failed;
    job.failure_reason = "permanent failure";

    store.add(job);

    assert(store.size() == 1);
    assert(!store.empty());

    auto jobs = store.jobs();

    assert(jobs.size() == 1);
    assert(jobs[0].job_id == 42);
    assert(jobs[0].status == taskforge::JobStatus::DeadLettered);
    assert(jobs[0].failure_reason == "permanent failure");
}

int main() {
    test_dead_letter_store_adds_job();

    return 0;
}