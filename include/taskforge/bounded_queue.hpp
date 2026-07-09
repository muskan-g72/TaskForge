#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>

namespace taskforge {

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("BoundedQueue capacity must be greater than zero");
        }
    }

    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);

        not_full_.wait(lock, [this] {
            return shutdown_ || queue_.size() < capacity_;
        });

        if (shutdown_) {
            return false;
        }

        queue_.push(std::move(item));
        not_empty_.notify_one();
        return true;
    }
    bool try_push(T item) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_ || queue_.size() >= capacity_) {
        return false;
    }

    queue_.push(std::move(item));
    not_empty_.notify_one();
    return true;
}

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);

        not_empty_.wait(lock, [this] {
            return shutdown_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        not_full_.notify_one();
        return true;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }

        not_full_.notify_all();
        not_empty_.notify_all();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool is_shutdown() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shutdown_;
    }

private:
    std::size_t capacity_;
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    bool shutdown_ = false;
};

}  // namespace taskforge

