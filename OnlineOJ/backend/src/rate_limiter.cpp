#include "rate_limiter.hpp"

RateLimiter& RateLimiter::instance() {
    static RateLimiter limiter;
    return limiter;
}

void RateLimiter::init(int max_per_min) {
    max_per_min_ = max_per_min;
}

bool RateLimiter::try_consume(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto& w = windows_[user_id];

    if (now > w.reset_time || w.count == 0) {
        w.count = 1;
        w.reset_time = now + std::chrono::minutes(1);
        return true;
    }

    if (w.count >= max_per_min_) return false;
    w.count++;
    return true;
}

void RateLimiter::refund(int user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = windows_.find(user_id);
    if (it != windows_.end() && it->second.count > 0) {
        it->second.count--;
    }
}
