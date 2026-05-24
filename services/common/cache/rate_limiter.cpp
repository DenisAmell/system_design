#include "cache/rate_limiter.hpp"

#include <algorithm>
#include <chrono>

namespace taxi::cache {

RateLimiter::RateLimiter(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : userver::components::ComponentBase(cfg, ctx),
      redis_(ctx.FindComponent<RedisClient>()) {}

RateLimiter::Decision RateLimiter::Acquire(std::string_view bucket_key,
                                           int64_t limit,
                                           int window_seconds) {
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    const auto window_start = (now / window_seconds) * window_seconds;
    const auto reset_in = window_seconds - (now - window_start);

    std::string key;
    key.reserve(bucket_key.size() + 22);
    key.append(bucket_key);
    key.push_back(':');
    key.append(std::to_string(window_start));

    const auto count = redis_.Incr(key);
    if (count == 1) {
        redis_.Expire(key, window_seconds);
    }

    Decision d;
    d.limit         = limit;
    d.remaining     = std::max<int64_t>(0, limit - count);
    d.reset_seconds = reset_in;
    d.allowed = (count == 0) || (count <= limit);
    return d;
}

}  // namespace taxi::cache
