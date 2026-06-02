#pragma once

#include <unordered_map>
#include <mutex>
#include <chrono>

class RateLimiter {
public:
    static RateLimiter& instance();
    void init(int max_per_min);
    bool try_consume(int user_id);
    void refund(int user_id);

private:
    RateLimiter() = default;

    struct Window {
        int count = 0;
        std::chrono::steady_clock::time_point reset_time;
    };

    std::mutex mutex_;
    std::unordered_map<int, Window> windows_;
    int max_per_min_ = 10;
};
