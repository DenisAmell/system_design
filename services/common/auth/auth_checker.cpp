#include "auth/auth_checker.hpp"

#include <string>

#include <userver/logging/log.hpp>
#include <userver/server/handlers/exceptions.hpp>

namespace taxi::auth {

namespace {
constexpr std::string_view kBearerPrefix = "Bearer ";
}

userver::server::handlers::auth::AuthCheckResult JwtChecker::CheckAuth(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    using Result = userver::server::handlers::auth::AuthCheckResult;
    using userver::server::handlers::HandlerErrorCode;

    const auto& header = request.GetHeader("Authorization");
    if (header.size() <= kBearerPrefix.size() ||
        std::string_view{header}.substr(0, kBearerPrefix.size()) !=
            kBearerPrefix) {
        return Result{
            .status = Result::Status::kTokenNotFound,
            .reason = std::string{"missing or malformed Authorization header"},
            .code   = HandlerErrorCode::kUnauthorized,
        };
    }

    const auto token =
        std::string_view{header}.substr(kBearerPrefix.size());
    auto login = auth_.VerifyToken(token);
    if (!login) {
        return Result{
            .status = Result::Status::kInvalidToken,
            .reason = std::string{"JWT signature or expiry check failed"},
            .code   = HandlerErrorCode::kUnauthorized,
        };
    }

    LOG_DEBUG() << "authenticated login " << *login;
    context.SetData(std::string{kAuthLoginKey}, std::move(*login));
    return {};
}

userver::server::handlers::auth::AuthCheckerBasePtr
JwtCheckerFactory::MakeAuthChecker(
    const userver::server::handlers::auth::HandlerAuthConfig&) const {
    return std::make_shared<JwtChecker>(auth_);
}

const std::string& GetAuthLogin(
    const userver::server::request::RequestContext& ctx) {
    return ctx.GetData<std::string>(std::string{kAuthLoginKey});
}

}  // namespace taxi::auth
