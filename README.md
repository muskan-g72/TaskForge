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

## Build

```bash
cmake -S . -B build
cmake --build build
```

---

## Run

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

### Benchmark Options

| Option | Description |
|--------|-------------|
| `--workers` | Number of worker threads |
| `--queue-capacity` | Maximum queue size |
| `--jobs` | Total jobs submitted |
| `--fast-percent` | Percentage of fast jobs |
| `--slow-percent` | Percentage of slow jobs |
| `--fail-percent` | Percentage of failing jobs |
| `--timeout-percent` | Percentage of timeout jobs |

> Percentages must add up to **100**.

---

## Benchmark Results

### Worker Scaling

| Workers | Throughput (jobs/s) | Avg Latency (ms) | P95 Latency (ms) |
|---------:|--------------------:|-----------------:|-----------------:|
| 1 | 62.72 | 1539.20 | 1637 |
| 2 | 125.96 | 770.48 | 812 |
| 4 | 245.16 | 402.15 | 423 |
| 8 | 484.03 | 210.20 | 248 |

### Queue Capacity Impact

| Capacity | Throughput (jobs/s) | Avg Latency (ms) | P95 Latency (ms) |
|---------:|--------------------:|-----------------:|-----------------:|
| 10 | 246.43 | 59.50 | 77 |
| 100 | 237.64 | 416.58 | 469 |
| 1000 | 229.78 | 2171.88 | 4320 |

**Observations**

- Throughput scales nearly linearly as worker count increases.
- Smaller queues reduce waiting time and improve latency.
- Larger queues buffer more work but increase average and tail latency.

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

---