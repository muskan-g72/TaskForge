#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace taskforge {

enum class JobStatus {
    Created,
    Queued,
    Running,
    Completed,
    Failed,
    TimedOut,
    Cancelled,
    DeadLettered
};

struct Job {
    std::uint64_t job_id = 0;

    int retry_count = 0;
    int max_retries = 3;
    int worker_id = -1;

    // 0 means timeout disabled for this job.
    int timeout_ms = 0;

    JobStatus status = JobStatus::Created;

    std::string failure_reason;

    std::chrono::steady_clock::time_point created_at = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point started_at{};
    std::chrono::steady_clock::time_point completed_at{};

    std::function<void()> task;
};

}  