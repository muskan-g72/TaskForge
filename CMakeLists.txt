cmake_minimum_required(VERSION 3.16)

project(TaskForge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(taskforge
    src/thread_pool.cpp
    src/scheduler.cpp
    src/retry_policy.cpp
    src/dead_letter_store.cpp
    src/logger.cpp
    src/metrics.cpp
)

target_include_directories(taskforge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(taskforge PUBLIC
    Threads::Threads
)

add_executable(taskforge_demo
    src/main.cpp
)

target_link_libraries(taskforge_demo PRIVATE
    taskforge
)
add_executable(taskforge_benchmark
    benchmarks/benchmark_driver.cpp
)

target_link_libraries(taskforge_benchmark PRIVATE
    taskforge
)
enable_testing()

add_executable(test_bounded_queue
    tests/test_bounded_queue.cpp
)

target_link_libraries(test_bounded_queue PRIVATE
    taskforge
)

add_test(NAME bounded_queue_tests COMMAND test_bounded_queue)

add_executable(test_thread_pool
    tests/test_thread_pool.cpp
)

target_link_libraries(test_thread_pool PRIVATE
    taskforge
)

add_test(NAME thread_pool_tests COMMAND test_thread_pool)

add_executable(test_scheduler
    tests/test_scheduler.cpp
)

target_link_libraries(test_scheduler PRIVATE
    taskforge
)

add_test(NAME scheduler_tests COMMAND test_scheduler)

add_executable(test_retry_policy
    tests/test_retry_policy.cpp
)

target_link_libraries(test_retry_policy PRIVATE
    taskforge
)

add_test(NAME retry_policy_tests COMMAND test_retry_policy)

add_executable(test_dead_letter_store
    tests/test_dead_letter_store.cpp
)

target_link_libraries(test_dead_letter_store PRIVATE
    taskforge
)

add_test(NAME dead_letter_store_tests COMMAND test_dead_letter_store)

add_executable(test_failure_handling
    tests/test_failure_handling.cpp
)

target_link_libraries(test_failure_handling PRIVATE
    taskforge
)

add_test(NAME failure_handling_tests COMMAND test_failure_handling)

add_executable(test_timeout
    tests/test_timeout.cpp
)

target_link_libraries(test_timeout PRIVATE
    taskforge
)

add_test(NAME timeout_tests COMMAND test_timeout)
add_executable(test_logger
    tests/test_logger.cpp
)

target_link_libraries(test_logger PRIVATE
    taskforge
)

add_test(NAME logger_tests COMMAND test_logger)

add_executable(test_logging_integration
    tests/test_logging_integration.cpp
)

target_link_libraries(test_logging_integration PRIVATE
    taskforge
)

add_test(NAME logging_integration_tests COMMAND test_logging_integration)
add_executable(test_metrics
    tests/test_metrics.cpp
)

target_link_libraries(test_metrics PRIVATE
    taskforge
)

add_test(NAME metrics_tests COMMAND test_metrics)

add_executable(test_metrics_integration
    tests/test_metrics_integration.cpp
)

target_link_libraries(test_metrics_integration PRIVATE
    taskforge
)

add_test(NAME metrics_integration_tests COMMAND test_metrics_integration)