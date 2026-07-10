#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "taskforge/job.hpp"
#include "taskforge/metrics.hpp"
#include "taskforge/scheduler.hpp"

namespace {

std::atomic<std::uint64_t> cpu_sink{0};

enum class WorkloadType {
    Io,
    Cpu
};

struct BenchmarkConfig {
    std::size_t workers = 4;
    std::size_t queue_capacity = 100;
    int jobs = 1000;
    int runs = 1;

    int fast_percent = 70;
    int slow_percent = 20;
    int fail_percent = 5;
    int timeout_percent = 5;

    WorkloadType workload = WorkloadType::Io;
};

struct RunResult {
    taskforge::MetricsSnapshot snapshot;
};

int parse_int(const std::string& value, const std::string& flag_name) {
    try {
        return std::stoi(value);
    } catch (...) {
        throw std::invalid_argument("invalid integer for " + flag_name + ": " + value);
    }
}

std::size_t parse_size(const std::string& value, const std::string& flag_name) {
    const int parsed = parse_int(value, flag_name);

    if (parsed <= 0) {
        throw std::invalid_argument(flag_name + " must be greater than zero");
    }

    return static_cast<std::size_t>(parsed);
}

WorkloadType parse_workload(const std::string& value) {
    if (value == "io") {
        return WorkloadType::Io;
    }

    if (value == "cpu") {
        return WorkloadType::Cpu;
    }

    throw std::invalid_argument("--workload must be either io or cpu");
}

std::string workload_name(WorkloadType workload) {
    if (workload == WorkloadType::Cpu) {
        return "cpu";
    }

    return "io";
}

BenchmarkConfig parse_args(int argc, char** argv) {
    BenchmarkConfig config;

    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];

        auto require_value = [&](const std::string& flag_name) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument("missing value for " + flag_name);
            }

            ++i;
            return argv[i];
        };

        if (flag == "--workers") {
            config.workers = parse_size(require_value(flag), flag);
        } else if (flag == "--queue-capacity") {
            config.queue_capacity = parse_size(require_value(flag), flag);
        } else if (flag == "--jobs") {
            config.jobs = parse_int(require_value(flag), flag);
        } else if (flag == "--runs") {
            config.runs = parse_int(require_value(flag), flag);
        } else if (flag == "--fast-percent") {
            config.fast_percent = parse_int(require_value(flag), flag);
        } else if (flag == "--slow-percent") {
            config.slow_percent = parse_int(require_value(flag), flag);
        } else if (flag == "--fail-percent") {
            config.fail_percent = parse_int(require_value(flag), flag);
        } else if (flag == "--timeout-percent") {
            config.timeout_percent = parse_int(require_value(flag), flag);
        } else if (flag == "--workload") {
            config.workload = parse_workload(require_value(flag));
        } else if (flag == "--help") {
            std::cout
                << "Usage: taskforge_benchmark [options]\n"
                << "Options:\n"
                << "  --workers N\n"
                << "  --queue-capacity N\n"
                << "  --jobs N\n"
                << "  --runs N\n"
                << "  --workload io|cpu\n"
                << "  --fast-percent N\n"
                << "  --slow-percent N\n"
                << "  --fail-percent N\n"
                << "  --timeout-percent N\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown flag: " + flag);
        }
    }

    if (config.jobs <= 0) {
        throw std::invalid_argument("--jobs must be greater than zero");
    }

    if (config.runs <= 0) {
        throw std::invalid_argument("--runs must be greater than zero");
    }

    const int total_percent =
        config.fast_percent + config.slow_percent + config.fail_percent + config.timeout_percent;

    if (total_percent != 100) {
        throw std::invalid_argument("job mix percentages must add up to 100");
    }

    return config;
}

void cpu_burn(std::uint64_t iterations) {
    std::uint64_t result = 0;

    for (std::uint64_t i = 0; i < iterations; ++i) {
        result += ((i * i) + 17) % 97;
    }

    cpu_sink.fetch_xor(result, std::memory_order_relaxed);
}

taskforge::Job make_fast_job(std::uint64_t job_id, WorkloadType workload) {
    taskforge::Job job;
    job.job_id = job_id;

    if (workload == WorkloadType::Cpu) {
        job.task = [] {
            cpu_burn(150000);
        };
    } else {
        job.task = [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        };
    }

    return job;
}

taskforge::Job make_slow_job(std::uint64_t job_id, WorkloadType workload) {
    taskforge::Job job;
    job.job_id = job_id;

    if (workload == WorkloadType::Cpu) {
        job.task = [] {
            cpu_burn(1200000);
        };
    } else {
        job.task = [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };
    }

    return job;
}

taskforge::Job make_failing_job(std::uint64_t job_id, WorkloadType workload) {
    taskforge::Job job;
    job.job_id = job_id;
    job.max_retries = 2;

    if (workload == WorkloadType::Cpu) {
        job.task = [] {
            cpu_burn(150000);
            throw std::runtime_error("benchmark injected cpu failure");
        };
    } else {
        job.task = [] {
            throw std::runtime_error("benchmark injected failure");
        };
    }

    return job;
}

taskforge::Job make_timeout_job(std::uint64_t job_id, WorkloadType workload) {
    taskforge::Job job;
    job.job_id = job_id;
    job.max_retries = 1;

    if (workload == WorkloadType::Cpu) {
        job.timeout_ms = 1;
        job.task = [] {
            cpu_burn(2000000);
        };
    } else {
        job.timeout_ms = 5;
        job.task = [] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        };
    }

    return job;
}

taskforge::Job make_job_from_mix(std::uint64_t job_id, const BenchmarkConfig& config) {
    const int bucket = static_cast<int>(job_id % 100);

    if (bucket < config.fast_percent) {
        return make_fast_job(job_id, config.workload);
    }

    if (bucket < config.fast_percent + config.slow_percent) {
        return make_slow_job(job_id, config.workload);
    }

    if (bucket < config.fast_percent + config.slow_percent + config.fail_percent) {
        return make_failing_job(job_id, config.workload);
    }

    return make_timeout_job(job_id, config.workload);
}

void print_config(const BenchmarkConfig& config) {
    std::cout << "config_workers=" << config.workers << '\n';
    std::cout << "config_queue_capacity=" << config.queue_capacity << '\n';
    std::cout << "config_jobs=" << config.jobs << '\n';
    std::cout << "config_runs=" << config.runs << '\n';
    std::cout << "config_workload=" << workload_name(config.workload) << '\n';
    std::cout << "config_fast_percent=" << config.fast_percent << '\n';
    std::cout << "config_slow_percent=" << config.slow_percent << '\n';
    std::cout << "config_fail_percent=" << config.fail_percent << '\n';
    std::cout << "config_timeout_percent=" << config.timeout_percent << '\n';
}

void print_metrics(const taskforge::MetricsSnapshot& snapshot) {
    std::cout << "submitted_jobs=" << snapshot.submitted_jobs << '\n';
    std::cout << "completed_jobs=" << snapshot.completed_jobs << '\n';
    std::cout << "failed_jobs=" << snapshot.failed_jobs << '\n';
    std::cout << "timed_out_jobs=" << snapshot.timed_out_jobs << '\n';
    std::cout << "retried_jobs=" << snapshot.retried_jobs << '\n';
    std::cout << "dead_lettered_jobs=" << snapshot.dead_lettered_jobs << '\n';
    std::cout << "average_latency_ms=" << snapshot.average_latency_ms << '\n';
    std::cout << "p95_latency_ms=" << snapshot.p95_latency_ms << '\n';
    std::cout << "throughput_jobs_per_sec=" << snapshot.throughput_jobs_per_sec << '\n';
}

RunResult run_once(const BenchmarkConfig& config) {
    taskforge::Scheduler scheduler(config.workers, config.queue_capacity);

    for (int i = 0; i < config.jobs; ++i) {
        const auto job_id = static_cast<std::uint64_t>(i + 1);
        taskforge::Job job = make_job_from_mix(job_id, config);

        if (!scheduler.submit(job)) {
            std::cerr << "failed_to_submit_job=" << job_id << '\n';
        }
    }

    scheduler.shutdown_drain();

    RunResult result;
    result.snapshot = scheduler.metrics()->snapshot();
    return result;
}

double mean(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }

    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / static_cast<double>(values.size());
}

double stddev(const std::vector<double>& values) {
    if (values.size() <= 1) {
        return 0.0;
    }

    const double avg = mean(values);
    double squared_diff_sum = 0.0;

    for (double value : values) {
        const double diff = value - avg;
        squared_diff_sum += diff * diff;
    }

    return std::sqrt(squared_diff_sum / static_cast<double>(values.size() - 1));
}

void print_multi_run_summary(const std::vector<RunResult>& results) {
    std::vector<double> throughputs;
    std::vector<double> average_latencies;
    std::vector<double> p95_latencies;

    for (const auto& result : results) {
        throughputs.push_back(result.snapshot.throughput_jobs_per_sec);
        average_latencies.push_back(result.snapshot.average_latency_ms);
        p95_latencies.push_back(result.snapshot.p95_latency_ms);
    }

    std::cout << "throughput_mean=" << mean(throughputs) << '\n';
    std::cout << "throughput_stddev=" << stddev(throughputs) << '\n';
    std::cout << "average_latency_mean_ms=" << mean(average_latencies) << '\n';
    std::cout << "average_latency_stddev_ms=" << stddev(average_latencies) << '\n';
    std::cout << "p95_latency_mean_ms=" << mean(p95_latencies) << '\n';
    std::cout << "p95_latency_stddev_ms=" << stddev(p95_latencies) << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const BenchmarkConfig config = parse_args(argc, argv);

        print_config(config);

        std::vector<RunResult> results;
        results.reserve(static_cast<std::size_t>(config.runs));

        for (int run = 1; run <= config.runs; ++run) {
            const RunResult result = run_once(config);
            results.push_back(result);

            std::cout << "run=" << run << '\n';
            print_metrics(result.snapshot);
        }

        if (config.runs > 1) {
            print_multi_run_summary(results);
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error=" << error.what() << '\n';
        return 1;
    }
}