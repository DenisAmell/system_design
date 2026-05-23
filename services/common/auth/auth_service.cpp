#include "auth/auth_service.hpp"

#include <cstdlib>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "jwt.hpp"

namespace taxi::auth {

namespace {

std::string ResolveSecret(const userver::components::ComponentConfig& cfg) {
    if (const char* env = std::getenv("JWT_SECRET"); env && *env) {
        return env;
    }
    return cfg["jwt_secret"].As<std::string>("change-me-in-prod");
}

std::string ResolvePepper(const userver::components::ComponentConfig& cfg) {
    if (const char* env = std::getenv("PASSWORD_PEPPER"); env && *env) {
        return env;
    }
    return cfg["password_pepper"].As<std::string>("taxi-svc-v1");
}

}  // namespace

AuthService::AuthService(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : userver::components::ComponentBase(cfg, ctx),
      secret_(ResolveSecret(cfg)),
      password_pepper_(ResolvePepper(cfg)),
      ttl_seconds_(cfg["jwt_ttl_seconds"].As<int64_t>(3600)) {}

std::string AuthService::IssueToken(std::string_view login) const {
    return jwt::Encode(login, secret_, ttl_seconds_);
}

std::optional<std::string> AuthService::VerifyToken(
    std::string_view token) const {
    return jwt::Decode(token, secret_);
}

std::string AuthService::HashPassword(std::string_view password) const {
    return jwt::HashPassword(password, password_pepper_);
}

userver::yaml_config::Schema AuthService::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
type: object
description: JWT secret/TTL provider for the bearer auth checker.
additionalProperties: false
properties:
    jwt_secret:
        type: string
        description: HMAC secret for signing JWT tokens (overridden by JWT_SECRET env).
        defaultDescription: change-me-in-prod
    jwt_ttl_seconds:
        type: integer
        description: Token time-to-live in seconds.
        defaultDescription: 3600
    password_pepper:
        type: string
        description: Pepper appended to passwords before SHA-256 hashing (overridden by PASSWORD_PEPPER env).
        defaultDescription: taxi-svc-v1
)");
}

}  // namespace taxi::auth
