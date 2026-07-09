#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace taskforge {

struct MetricsSnapshot {
    std::uint64_t submitted_jobs = 0;
    std::uint64_t completed_jobs = 0;
    std::uint64_t failed_jobs = 0;
    std::uint64_t timed_out_jobs = 0;
    std::uint64_t retried_jobs = 0;
    std::uint64_t dead_lettered_jobs = 0;

    double average_latency_ms = 0.0;
    double p95_latency_ms = 0.0;
    double throughput_jobs_per_sec = 0.0;
};

class Metrics {
public:
    Metrics();

    void record_submitted();
    void record_completed(std::chrono::milliseconds latency);
    void record_failed();
    void record_timed_out();
    void record_retried();
    void record_dead_lettered(std::chrono::milliseconds latency);

    MetricsSnapshot snapshot() const;

private:
    double calculate_p95_locked() const;
    double calculate_throughput_locked(std::uint64_t terminal_jobs) const;

    mutable std::mutex mutex_;

    std::uint64_t submitted_jobs_ = 0;
    std::uint64_t completed_jobs_ = 0;
    std::uint64_t failed_jobs_ = 0;
    std::uint64_t timed_out_jobs_ = 0;
    std::uint64_t retried_jobs_ = 0;
    std::uint64_t dead_lettered_jobs_ = 0;

    std::uint64_t total_latency_ms_ = 0;
    std::vector<std::uint64_t> latencies_ms_;

    std::chrono::steady_clock::time_point started_at_;
};

}  