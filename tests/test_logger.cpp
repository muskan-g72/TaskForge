#include <cassert>
#include <sstream>
#include <string>

#include "taskforge/logger.hpp"

void test_logger_writes_job_queued_event() {
    std::ostringstream output;
    taskforge::Logger logger(output);

    logger.job_queued(42, 0);

    assert(output.str() == "{\"event\":\"job_queued\",\"job_id\":42,\"retry_count\":0}\n");
}

void test_logger_escapes_json_reason() {
    std::ostringstream output;
    taskforge::Logger logger(output);

    logger.job_failed(7, 1, "bad \"input\"");

    assert(output.str() == "{\"event\":\"job_failed\",\"job_id\":7,\"worker_id\":1,\"reason\":\"bad \\\"input\\\"\"}\n");
}

int main() {
    test_logger_writes_job_queued_event();
    test_logger_escapes_json_reason();

    return 0;
}