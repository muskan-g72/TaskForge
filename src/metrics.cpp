#include "taskforge/metrics.hpp"

#include <algorithm>

namespace taskforge {

Metrics::Metrics() : started_at_(std::chrono::steady_clock::now()) {}

void Metrics::record_submitted() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++submitted_jobs_;
}

void Metrics::record_completed(std::chrono::milliseconds latency) {
    std::lock_guard<std::mutex> lock(mutex_);

    ++completed_jobs_;

    const auto latency_ms = static_cast<std::uint64_t>(latency.count());
    total_latency_ms_ += latency_ms;
    latencies_ms_.push_back(latency_ms);
}

void Metrics::record_failed() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++failed_jobs_;
}

void Metrics::record_timed_out() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++timed_out_jobs_;
}

void Metrics::record_retried() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++retried_jobs_;
}

void Metrics::record_dead_lettered(std::chrono::milliseconds latency) {
    std::lock_guard<std::mutex> lock(mutex_);

    ++dead_lettered_jobs_;

    const auto latency_ms = static_cast<std::uint64_t>(latency.count());
    total_latency_ms_ += latency_ms;
    latencies_ms_.push_back(latency_ms);
}

MetricsSnapshot Metrics::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    MetricsSnapshot snapshot;

    snapshot.submitted_jobs = submitted_jobs_;
    snapshot.completed_jobs = completed_jobs_;
    snapshot.failed_jobs = failed_jobs_;
    snapshot.timed_out_jobs = timed_out_jobs_;
    snapshot.retried_jobs = retried_jobs_;
    snapshot.dead_lettered_jobs = dead_lettered_jobs_;

    const auto terminal_jobs = completed_jobs_ + dead_lettered_jobs_;

    if (terminal_jobs > 0) {
        snapshot.average_latency_ms =
            static_cast<double>(total_latency_ms_) / static_cast<double>(terminal_jobs);
    }

    snapshot.p95_latency_ms = calculate_p95_locked();
    snapshot.throughput_jobs_per_sec = calculate_throughput_locked(terminal_jobs);

    return snapshot;
}

double Metrics::calculate_p95_locked() const {
    if (latencies_ms_.empty()) {
        return 0.0;
    }

    std::vector<std::uint64_t> sorted = latencies_ms_;
    std::sort(sorted.begin(), sorted.end());

    const std::size_t index =
        static_cast<std::size_t>(0.95 * static_cast<double>(sorted.size() - 1));

    return static_cast<double>(sorted[index]);
}

double Metrics::calculate_throughput_locked(std::uint64_t terminal_jobs) const {
    if (terminal_jobs == 0) {
        return 0.0;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - started_at_).count();

    if (elapsed_ms <= 0) {
        return 0.0;
    }

    return static_cast<double>(terminal_jobs) / (static_cast<double>(elapsed_ms) / 1000.0);
}

}