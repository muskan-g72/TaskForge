#include <cassert>
#include <chrono>

#include "taskforge/metrics.hpp"

void test_metrics_records_completed_jobs() {
    taskforge::Metrics metrics;

    metrics.record_submitted();
    metrics.record_completed(std::chrono::milliseconds(10));

    auto snapshot = metrics.snapshot();

    assert(snapshot.submitted_jobs == 1);
    assert(snapshot.completed_jobs == 1);
    assert(snapshot.average_latency_ms == 10.0);
    assert(snapshot.p95_latency_ms == 10.0);
}

void test_metrics_records_retries_and_dead_letters() {
    taskforge::Metrics metrics;

    metrics.record_submitted();
    metrics.record_failed();
    metrics.record_retried();
    metrics.record_failed();
    metrics.record_dead_lettered(std::chrono::milliseconds(25));

    auto snapshot = metrics.snapshot();

    assert(snapshot.submitted_jobs == 1);
    assert(snapshot.failed_jobs == 2);
    assert(snapshot.retried_jobs == 1);
    assert(snapshot.dead_lettered_jobs == 1);
    assert(snapshot.average_latency_ms == 25.0);
}

int main() {
    test_metrics_records_completed_jobs();
    test_metrics_records_retries_and_dead_letters();

    return 0;
}