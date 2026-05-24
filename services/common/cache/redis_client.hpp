#pragma once

// Thin userver component wrapping a single hiredis connection. The cache and
// rate-limiter both go through this — that way the lab keeps a single point
// where Redis ↔ service boundary lives.
//
// Operations are synchronous (hiredis sync API). On a local docker network
// each call is sub-millisecond, so we serialize them under one mutex rather
// than maintain a pool. If Redis is unavailable, methods log a warning and
// return "miss" semantics — callers gracefully fall back to the source of
// truth (DB / no rate-limit). This keeps the service operational under cache
// outages.

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

struct redisContext;  // forward-decl from <hiredis/hiredis.h>

namespace taxi::cache {

class RedisClient final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "redis-client";

    RedisClient(const userver::components::ComponentConfig&,
                const userver::components::ComponentContext&);
    ~RedisClient() override;

    // Returns nullopt for missing key OR if Redis is unreachable
    // (graceful degradation).
    std::optional<std::string> Get(std::string_view key);

    // Best-effort SET with EX <ttl_seconds>. Silently dropped on Redis error.
    void SetEx(std::string_view key, std::string_view value, int ttl_seconds);

    // Best-effort DEL.
    void Del(std::string_view key);

    // INCR; returns the value after increment. Returns 0 if Redis is down
    // (so rate-limiter degrades to allow-all).
    int64_t Incr(std::string_view key);

    // EXPIRE; returns true on success.
    bool Expire(std::string_view key, int seconds);

    // TTL in seconds. -1 = no expiry, -2 = missing, or 0 on Redis error.
    int64_t Ttl(std::string_view key);

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    void Connect();
    void Disconnect();
    bool EnsureConnected();  // lazy reconnect after a failure

    std::string host_;
    int port_;
    int connect_timeout_ms_;

    std::mutex mu_;
    redisContext* ctx_{nullptr};
};

}  // namespace taxi::cache
