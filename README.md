# TaskForge

> A concurrent task scheduler built in modern C++ featuring a bounded blocking queue, fixed-size thread pool, retry handling, timeout detection, structured logging, runtime metrics and a configurable benchmark driver.

---

## Features

### Concurrency
- Fixed-size worker pool using `std::thread`
- Thread-safe bounded blocking queue
- Backpressure when the queue reaches capacity
- Graceful drain shutdown

### Reliability
- Configurable retry policy
- Timeout detection
- Dead-letter queue for permanently failed jobs

### Observability
- Structured JSON-lines logging
- Runtime metrics
- Job and worker lifecycle tracking

### Performance
- Configurable benchmark driver
- Worker scaling and queue capacity analysis

---

## Tech Stack

- C++17
- CMake
- `std::thread`
- `std::mutex`
- `std::condition_variable`
- CTest

---

## Architecture

```text
                Producer
                   │
                   ▼
             ┌───────────┐
             │ Scheduler │
             └─────┬─────┘
                   │
                   ▼
      ┌────────────────────────┐
      │ Bounded Blocking Queue │
      └──────────┬─────────────┘
                 │
                 ▼
         ┌────────────────┐
         │ Worker Threads │
         └────────┬───────┘
                  │
                  ▼
          ┌──────────────┐
          │ Job Execution│
          └──────┬───────┘
                 │
       ┌─────────┴─────────┐
       ▼                   ▼
   Completed        Retry / Dead Letter
```

---

## Build Environment

Developed and benchmarked on Windows using MSYS2 UCRT64 with MinGW-w64 GCC.

Native Windows/MinGW-w64 does not provide ThreadSanitizer runtime support, so ThreadSanitizer validation was performed under Ubuntu/WSL2.

```bash
cmake -S . -B build-tsan -DENABLE_TSAN=ON
cmake --build build-tsan
ctest --test-dir build-tsan --output-on-failure
```

## Run
### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

### Run Demo

```bash
./build/taskforge_demo
```

### Run Benchmark

```bash
./build/taskforge_benchmark --workers 4 --queue-capacity 100 --jobs 1000
```

## Benchmark

The benchmark driver supports both **I/O-bound** and **CPU-bound** workloads with configurable worker count, queue capacity, workload mix, and multiple benchmark runs.

### Benchmark Options

| Option | Description |
|--------|-------------|
| `--workers` | Number of worker threads |
| `--queue-capacity` | Maximum queue size |
| `--jobs` | Total jobs submitted |
| `--workload` | `io` or `cpu` |
| `--runs` | Number of benchmark runs |
| `--fast-percent` | Percentage of fast jobs |
| `--slow-percent` | Percentage of slow jobs |
| `--fail-percent` | Percentage of failing jobs |
| `--timeout-percent` | Percentage of timeout jobs |

> Percentages must add up to **100**.

Results are reported as **mean ± standard deviation** across **5 runs**.

The I/O-bound benchmark uses sleep-based simulated work, so near-linear scaling is expected as more workers overlap waiting time. CPU-bound results are included separately to show scheduler behavior under compute-heavy work.

### I/O-Bound Worker Scaling

Configuration: **1000 jobs**, **queue capacity = 100**

| Workers | Throughput (jobs/s) | Avg Latency (ms) | P95 Latency (ms) |
|--------:|--------------------:|-----------------:|-----------------:|
| 1 | 62.91 ± 0.54 | 1530.55 ± 13.92 | 1624.2 ± 43.64 |
| 2 | 125.44 ± 0.69 | 772.62 ± 4.37 | 831.6 ± 19.44 |
| 4 | 245.80 ± 4.04 | 401.68 ± 6.02 | 435.0 ± 19.80 |
| 8 | 480.97 ± 3.33 | 212.29 ± 1.71 | 341.0 ± 78.49 |

### CPU-Bound Worker Scaling

Configuration: **300 jobs**, **queue capacity = 100**

| Workers | Throughput (jobs/s) | Avg Latency (ms) | P95 Latency (ms) |
|--------:|--------------------:|-----------------:|-----------------:|
| 1 | 456.84 ± 43.25 | 148.80 ± 14.34 | 229.6 ± 15.08 |
| 2 | 1155.62 ± 144.02 | 62.84 ± 12.43 | 90.4 ± 18.96 |
| 4 | 2044.19 ± 197.73 | 35.54 ± 4.57 | 52.4 ± 6.31 |
| 8 | 2805.69 ± 266.53 | 25.90 ± 3.88 | 43.6 ± 5.59 |

### Queue Capacity Impact

Configuration: **4 workers**, **1000 I/O-bound jobs**, **5 runs**

| Queue Capacity | Throughput (jobs/s) | Avg Latency (ms) | P95 Latency (ms) |
|--------------:|--------------------:|-----------------:|-----------------:|
| 10 | 245.66 ± 5.32 | 59.76 ± 1.28 | 80.0 ± 6.20 |
| 100 | 246.36 ± 3.12 | 401.23 ± 6.28 | 441.4 ± 11.89 |
| 1000 | 230.34 ± 1.29 | 2179.70 ± 16.98 | 4296.8 ± 29.03 |


### Key Findings

- I/O-bound throughput scaled nearly linearly as worker count increased.
- CPU-bound workloads achieved higher throughput with additional worker threads, with gains becoming dependent on available CPU resources.
- Smaller queue capacities reduced waiting time and improved latency, while larger queues increased buffering at the cost of higher tail latency.
- Reporting results as **mean ± standard deviation** across multiple runs reduced the impact of run-to-run variability.

---

## Metrics

TaskForge tracks:

- Submitted jobs
- Completed jobs
- Failed jobs
- Timed-out jobs
- Retried jobs
- Dead-lettered jobs
- Average latency
- P95 latency
- Throughput (jobs/sec)

---

## Logging

Example JSON log entries:

```json
{"event":"job_queued","job_id":1}
{"event":"job_started","job_id":1,"worker_id":0}
{"event":"job_completed","job_id":1,"worker_id":0}
{"event":"job_failed","job_id":2,"worker_id":1}
{"event":"job_retried","job_id":2,"retry_count":1}
{"event":"job_dead_lettered","job_id":2}
```

---

## Test Coverage

- Bounded queue
- Thread pool
- Scheduler
- Retry handling
- Timeout handling
- Dead-letter queue
- Logging
- Metrics

---
## Current Limitations

- Single-machine scheduler
- In-memory queue (no persistence)
- No priority scheduling
- No work stealing

## Future Improvements

- Priority-based scheduling
- Work stealing between worker threads
- Disk-backed queue for persistence and crash recovery