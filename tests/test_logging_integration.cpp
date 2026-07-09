#include <cassert>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "taskforge/job.hpp"
#include "taskforge/logger.hpp"
#include "taskforge/scheduler.hpp"

void test_successful_job_emits_lifecycle_logs() {
    std::ostringstream output;
    auto logger = std::make_shared<taskforge::Logger>(output);

    taskforge::Scheduler scheduler(1, 10, logger);

    taskforge::Job job;
    job.job_id = 1;
    job.task = [] {};

    assert(scheduler.submit(job));

    scheduler.shutdown_drain();

    const std::string logs = output.str();

    assert(logs.find("\"event\":\"job_queued\"") != std::string::npos);
    assert(logs.find("\"event\":\"job_started\"") != std::string::npos);
    assert(logs.find("\"event\":\"job_completed\"") != std::string::npos);
}

void test_failing_job_emits_retry_and_dead_letter_logs() {
    std::ostringstream output;
    auto logger = std::make_shared<taskforge::Logger>(output);

    taskforge::Scheduler scheduler(1, 10, logger);

    taskforge::Job job;
    job.job_id = 2;
    job.max_retries = 1;
    job.task = [] {
        throw std::runtime_error("permanent failure");
    };

    assert(scheduler.submit(job));

    scheduler.shutdown_drain();

    const std::string logs = output.str();

    assert(logs.find("\"event\":\"job_failed\"") != std::string::npos);
    assert(logs.find("\"event\":\"job_retried\"") != std::string::npos);
    assert(logs.find("\"event\":\"job_dead_lettered\"") != std::string::npos);
}

int main() {
    test_successful_job_emits_lifecycle_logs();
    test_failing_job_emits_retry_and_dead_letter_logs();

    return 0;
}