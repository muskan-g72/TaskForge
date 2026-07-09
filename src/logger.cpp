#include "taskforge/logger.hpp"

#include <ostream>
#include <sstream>

namespace taskforge {

Logger::Logger(std::ostream& output) : output_(output) {}

void Logger::job_queued(std::uint64_t job_id, int retry_count) {
    std::ostringstream line;
    line << "{\"event\":\"job_queued\",\"job_id\":" << job_id
         << ",\"retry_count\":" << retry_count << "}";
    write_line(line.str());
}

void Logger::job_started(std::uint64_t job_id, int worker_id, int retry_count) {
    std::ostringstream line;
    line << "{\"event\":\"job_started\",\"job_id\":" << job_id
         << ",\"worker_id\":" << worker_id
         << ",\"retry_count\":" << retry_count << "}";
    write_line(line.str());
}

void Logger::job_completed(std::uint64_t job_id, int worker_id) {
    std::ostringstream line;
    line << "{\"event\":\"job_completed\",\"job_id\":" << job_id
         << ",\"worker_id\":" << worker_id << "}";
    write_line(line.str());
}

void Logger::job_failed(std::uint64_t job_id, int worker_id, const std::string& reason) {
    std::ostringstream line;
    line << "{\"event\":\"job_failed\",\"job_id\":" << job_id
         << ",\"worker_id\":" << worker_id
         << ",\"reason\":\"" << escape_json(reason) << "\"}";
    write_line(line.str());
}

void Logger::job_timed_out(std::uint64_t job_id, int worker_id, const std::string& reason) {
    std::ostringstream line;
    line << "{\"event\":\"job_timed_out\",\"job_id\":" << job_id
         << ",\"worker_id\":" << worker_id
         << ",\"reason\":\"" << escape_json(reason) << "\"}";
    write_line(line.str());
}

void Logger::job_retried(std::uint64_t job_id, int retry_count) {
    std::ostringstream line;
    line << "{\"event\":\"job_retried\",\"job_id\":" << job_id
         << ",\"retry_count\":" << retry_count << "}";
    write_line(line.str());
}

void Logger::job_dead_lettered(std::uint64_t job_id, const std::string& reason) {
    std::ostringstream line;
    line << "{\"event\":\"job_dead_lettered\",\"job_id\":" << job_id
         << ",\"reason\":\"" << escape_json(reason) << "\"}";
    write_line(line.str());
}

void Logger::write_line(const std::string& line) {
    std::lock_guard<std::mutex> lock(mutex_);
    output_ << line << '\n';
}

std::string Logger::escape_json(const std::string& value) {
    std::string escaped;

    for (char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }

    return escaped;
}

} 