#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/handler_auth_config.hpp>
#include <userver/server/request/request_context.hpp>

#include "auth/auth_service.hpp"

namespace taxi::auth {

inline constexpr std::string_view kAuthLoginKey = "taxi-auth-login";

class JwtChecker final
    : public userver::server::handlers::auth::AuthCheckerBase {
public:
    explicit JwtChecker(const AuthService& auth) : auth_{auth} {}

    [[nodiscard]] userver::server::handlers::auth::AuthCheckResult CheckAuth(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

    [[nodiscard]] bool SupportsUserAuth() const noexcept override {
        return true;
    }

private:
    const AuthService& auth_;
};

class JwtCheckerFactory final
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "bearer";

    explicit JwtCheckerFactory(
        const userver::components::ComponentContext& ctx)
        : auth_{ctx.FindComponent<AuthService>()} {}

    userver::server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const userver::server::handlers::auth::HandlerAuthConfig&)
        const override;

private:
    const AuthService& auth_;
};

const std::string& GetAuthLogin(
    const userver::server::request::RequestContext& ctx);

}  // namespace taxi::auth
