#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

namespace taxi::auth {

// Token issuer / verifier / password hasher. Every backend appends this
// component so that:
//   * user-service can mint JWTs in /v1/login;
//   * every service can verify incoming Bearer tokens via JwtChecker.
//
// The shared HS256 secret comes from env (`JWT_SECRET`) or the static config.
class AuthService final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "auth-service";

    AuthService(const userver::components::ComponentConfig&,
                const userver::components::ComponentContext&);

    std::string IssueToken(std::string_view login) const;
    std::optional<std::string> VerifyToken(std::string_view token) const;
    std::string HashPassword(std::string_view password) const;

    int64_t TokenTtlSeconds() const noexcept { return ttl_seconds_; }

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    std::string secret_;
    std::string password_pepper_;
    int64_t ttl_seconds_;
};

}  // namespace taxi::auth
