#pragma once
// Fixed-window per-IP rate limiter: N requests per 60s window. Thread-safe.

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace gateway {

class RateLimiter {
public:
    explicit RateLimiter(int perMin = 100) : perMin_(perMin) {}

    void setLimit(int perMin) { perMin_ = perMin; }

    // true = request allowed, false = respond 429.
    bool allow(const std::string& ip) {
        if (perMin_ <= 0) return true;  // 0 = disabled
        const long long now =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();
        std::lock_guard<std::mutex> lock(mutex_);
        if (buckets_.size() > 20000) purge(now);
        Bucket& b = buckets_[ip];
        if (b.windowStart == 0 || now - b.windowStart >= 60) {
            b.windowStart = now;
            b.count = 1;
            return true;
        }
        if (b.count >= perMin_) return false;
        ++b.count;
        return true;
    }

private:
    struct Bucket {
        long long windowStart = 0;
        int count = 0;
    };
    void purge(long long now) {
        for (auto it = buckets_.begin(); it != buckets_.end();) {
            if (now - it->second.windowStart >= 60)
                it = buckets_.erase(it);
            else
                ++it;
        }
    }

    int perMin_;
    std::mutex mutex_;
    std::unordered_map<std::string, Bucket> buckets_;
};

}  // namespace gateway
