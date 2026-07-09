#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>

#include "taskforge/bounded_queue.hpp"

using taskforge::BoundedQueue;

void test_fifo_order() {
    BoundedQueue<int> queue(3);

    assert(queue.push(10));
    assert(queue.push(20));
    assert(queue.push(30));

    int value = 0;
    assert(queue.pop(value));
    assert(value == 10);
    assert(queue.pop(value));
    assert(value == 20);
    assert(queue.pop(value));
    assert(value == 30);

    queue.shutdown();
}

void test_push_blocks_when_full() {
    BoundedQueue<int> queue(1);
    std::atomic<bool> producer_finished{false};

    assert(queue.push(1));

    std::thread producer([&] {
        queue.push(2);
        producer_finished = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(!producer_finished.load());

    int value = 0;
    assert(queue.pop(value));
    assert(value == 1);

    producer.join();
    assert(producer_finished.load());

    assert(queue.pop(value));
    assert(value == 2);

    queue.shutdown();
}

void test_pop_wakes_on_shutdown() {
    BoundedQueue<int> queue(2);
    std::atomic<bool> pop_returned{false};

    std::thread consumer([&] {
        int value = 0;
        bool result = queue.pop(value);
        assert(!result);
        pop_returned = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(!pop_returned.load());

    queue.shutdown();
    consumer.join();

    assert(pop_returned.load());
}

void test_concurrent_producers_consumers() {
    constexpr int producer_count = 4;
    constexpr int jobs_per_producer = 100;
    constexpr int total_jobs = producer_count * jobs_per_producer;

    BoundedQueue<int> queue(16);
    std::atomic<int> consumed_count{0};
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&] {
            int value = 0;
            while (queue.pop(value)) {
                ++consumed_count;
            }
        });
    }

    for (int producer_id = 0; producer_id < producer_count; ++producer_id) {
        producers.emplace_back([&, producer_id] {
            for (int i = 0; i < jobs_per_producer; ++i) {
                queue.push((producer_id * jobs_per_producer) + i);
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    queue.shutdown();

    for (auto& consumer : consumers) {
        consumer.join();
    }

    assert(consumed_count.load() == total_jobs);
}

int main() {
    test_fifo_order();
    test_push_blocks_when_full();
    test_pop_wakes_on_shutdown();
    test_concurrent_producers_consumers();
    return 0;
}

