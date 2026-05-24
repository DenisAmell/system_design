#include "cache/redis_client.hpp"

#include <cstdlib>
#include <cstring>

#include <hiredis/hiredis.h>

#include <userver/components/component_config.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace taxi::cache {

namespace {

struct ReplyDeleter {
    void operator()(redisReply* r) const noexcept {
        if (r) freeReplyObject(r);
    }
};
using ReplyPtr = std::unique_ptr<redisReply, ReplyDeleter>;

std::optional<std::string> ResolveEnv(const char* key) {
    if (const char* v = std::getenv(key); v && *v) return std::string{v};
    return std::nullopt;
}

}  // namespace

RedisClient::RedisClient(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : userver::components::ComponentBase(cfg, ctx),
      host_(ResolveEnv("REDIS_HOST").value_or(
          cfg["host"].As<std::string>("redis"))),
      port_(static_cast<int>(
          std::strtol(ResolveEnv("REDIS_PORT").value_or("0").c_str(), nullptr,
                      10))),
      connect_timeout_ms_(cfg["connect_timeout_ms"].As<int>(500)) {
    if (port_ == 0) port_ = cfg["port"].As<int>(6379);
    Connect();
    LOG_INFO() << "redis-client target " << host_ << ":" << port_
               << (ctx_ ? " (connected)" : " (offline — degrading)");
}

RedisClient::~RedisClient() { Disconnect(); }

void RedisClient::Connect() {
    struct timeval tv{
        connect_timeout_ms_ / 1000,
        (connect_timeout_ms_ % 1000) * 1000,
    };
    auto* c = redisConnectWithTimeout(host_.c_str(), port_, tv);
    if (!c || c->err) {
        if (c) {
            LOG_WARNING() << "redis connect failed: " << c->errstr;
            redisFree(c);
        } else {
            LOG_WARNING() << "redis connect failed: cannot allocate context";
        }
        ctx_ = nullptr;
        return;
    }
    // Snappy read/write timeouts; cache lookups must not stall request paths.
    redisSetTimeout(c, tv);
    ctx_ = c;
}

void RedisClient::Disconnect() {
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

bool RedisClient::EnsureConnected() {
    if (ctx_ && ctx_->err == 0) return true;
    if (ctx_) {
        Disconnect();
    }
    Connect();
    return ctx_ != nullptr;
}

std::optional<std::string> RedisClient::Get(std::string_view key) {
    std::lock_guard lock(mu_);
    if (!EnsureConnected()) return std::nullopt;

    ReplyPtr r{static_cast<redisReply*>(
        redisCommand(ctx_, "GET %b", key.data(), key.size()))};
    if (!r) {
        LOG_WARNING() << "redis GET error: " << ctx_->errstr;
        Disconnect();
        return std::nullopt;
    }
    if (r->type == REDIS_REPLY_NIL) return std::nullopt;
    if (r->type != REDIS_REPLY_STRING) return std::nullopt;
    return std::string(r->str, r->len);
}

void RedisClient::SetEx(std::string_view key, std::string_view value,
                        int ttl_seconds) {
    std::lock_guard lock(mu_);
    if (!EnsureConnected()) return;

    ReplyPtr r{static_cast<redisReply*>(
        redisCommand(ctx_, "SET %b %b EX %d",
                     key.data(), key.size(),
                     value.data(), value.size(),
                     ttl_seconds))};
    if (!r) {
        LOG_WARNING() << "redis SET error: " << ctx_->errstr;
        Disconnect();
    }
}

void RedisClient::Del(std::string_view key) {
    std::lock_guard lock(mu_);
    if (!EnsureConnected()) return;

    ReplyPtr r{static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %b", key.data(), key.size()))};
    if (!r) {
        LOG_WARNING() << "redis DEL error: " << ctx_->errstr;
        Disconnect();
    }
}

int64_t RedisClient::Incr(std::string_view key) {
    std::lock_guard lock(mu_);
    if (!EnsureConnected()) return 0;

    ReplyPtr r{static_cast<redisReply*>(
        redisCommand(ctx_, "INCR %b", key.data(), key.size()))};
    if (!r) {
        LOG_WARNING() << "redis INCR error: " << ctx_->errstr;
        Disconnect();
        return 0;
    }
    if (r->type == REDIS_REPLY_INTEGER) return r->integer;
    return 0;
}

bool RedisClient::Expire(std::string_view key, int seconds) {
    std::lock_guard lock(mu_);
    if (!EnsureConnected()) return false;

    ReplyPtr r{static_cast<redisReply*>(
        redisCommand(ctx_, "EXPIRE %b %d",
                     key.data(), key.size(), seconds))};
    if (!r) {
        LOG_WARNING() << "redis EXPIRE error: " << ctx_->errstr;
        Disconnect();
        return false;
    }
    return r->type == REDIS_REPLY_INTEGER && r->integer == 1;
}

int64_t RedisClient::Ttl(std::string_view key) {
    std::lock_guard lock(mu_);
    if (!EnsureConnected()) return 0;

    ReplyPtr r{static_cast<redisReply*>(
        redisCommand(ctx_, "TTL %b", key.data(), key.size()))};
    if (!r) {
        LOG_WARNING() << "redis TTL error: " << ctx_->errstr;
        Disconnect();
        return 0;
    }
    if (r->type == REDIS_REPLY_INTEGER) return r->integer;
    return 0;
}

userver::yaml_config::Schema RedisClient::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
type: object
description: Thin libhiredis client used by the cache and rate-limiter.
additionalProperties: false
properties:
    host:
        type: string
        description: Redis host. Overridable by REDIS_HOST env.
        defaultDescription: redis
    port:
        type: integer
        description: Redis port. Overridable by REDIS_PORT env.
        defaultDescription: 6379
    connect_timeout_ms:
        type: integer
        description: Connect / read / write timeout in milliseconds.
        defaultDescription: 500
)");
}

}  // namespace taxi::cache
