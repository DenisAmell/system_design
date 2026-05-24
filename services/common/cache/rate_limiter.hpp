#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

#include "cache/redis_client.hpp"

namespace taxi::cache {

class RateLimiter final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "rate-limiter";

    RateLimiter(const userver::components::ComponentConfig&,
                const userver::components::ComponentContext&);

    struct Decision {
        bool allowed{true};
        int64_t limit{0};
        int64_t remaining{0};
        int64_t reset_seconds{0};
    };

    Decision Acquire(std::string_view bucket_key,
                     int64_t limit,
                     int window_seconds);

private:
    RedisClient& redis_;
};

}  // namespace taxi::cache
