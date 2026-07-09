#pragma once

#include <cstdint>
#include <iosfwd>
#include <mutex>
#include <string>

namespace taskforge {

class Logger {
public:
    explicit Logger(std::ostream& output);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void job_queued(std::uint64_t job_id, int retry_count);
    void job_started(std::uint64_t job_id, int worker_id, int retry_count);
    void job_completed(std::uint64_t job_id, int worker_id);
    void job_failed(std::uint64_t job_id, int worker_id, const std::string& reason);
    void job_timed_out(std::uint64_t job_id, int worker_id, const std::string& reason);
    void job_retried(std::uint64_t job_id, int retry_count);
    void job_dead_lettered(std::uint64_t job_id, const std::string& reason);

private:
    void write_line(const std::string& line);
    static std::string escape_json(const std::string& value);

    std::ostream& output_;
    std::mutex mutex_;
};

}  