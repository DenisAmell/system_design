#include "handlers/handlers.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_status.hpp>

#include "dto/user_dto.hpp"

namespace taxi::user::handlers {

namespace {

using userver::server::handlers::ClientError;
using userver::server::handlers::ConflictError;
using userver::server::handlers::ExternalBody;
using userver::server::handlers::ResourceNotFound;
using userver::server::handlers::Unauthorized;

ExternalBody ErrorBody(std::string_view code, std::string_view message) {
    return ExternalBody{userver::formats::json::ToString(
        dto::ErrorJson(code, message))};
}

userver::formats::json::Value ParseJsonOrThrow(const std::string& body) {
    try {
        return userver::formats::json::FromString(body);
    } catch (const std::exception& e) {
        throw ClientError(ErrorBody("invalid_json", e.what()));
    }
}

}  // namespace


AuthLogin::AuthLogin(const userver::components::ComponentConfig& cfg,
                     const userver::components::ComponentContext& ctx)
    : userver::server::handlers::HttpHandlerBase(cfg, ctx),
      users_(ctx.FindComponent<repository::UserRepository>()),
      auth_(ctx.FindComponent<auth::AuthService>()) {}

std::string AuthLogin::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    req.GetHttpResponse().SetContentType("application/json");

    const auto body = ParseJsonOrThrow(req.RequestBody());
    const auto login    = body["login"].As<std::string>("");
    const auto password = body["password"].As<std::string>("");
    if (login.empty() || password.empty()) {
        throw ClientError(ErrorBody(
            "validation_failed", "login and password are required"));
    }

    auto user = users_.Get().GetByLogin(login);
    if (!user || user->password_hash != auth_.HashPassword(password)) {
        throw Unauthorized(ErrorBody(
            "invalid_credentials", "invalid login or password"));
    }

    return userver::formats::json::ToString(
        userver::formats::json::MakeObject(
            "token",      auth_.IssueToken(user->login),
            "token_type", std::string("Bearer"),
            "expires_in", auth_.TokenTtlSeconds()));
}


UserCreate::UserCreate(const userver::components::ComponentConfig& cfg,
                       const userver::components::ComponentContext& ctx)
    : userver::server::handlers::HttpHandlerBase(cfg, ctx),
      users_(ctx.FindComponent<repository::UserRepository>()),
      auth_(ctx.FindComponent<auth::AuthService>()) {}

std::string UserCreate::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    auto& resp = req.GetHttpResponse();
    resp.SetContentType("application/json");

    const auto body = ParseJsonOrThrow(req.RequestBody());

    User u;
    u.login         = body["login"].As<std::string>("");
    const auto pw   = body["password"].As<std::string>("");
    u.first_name    = body["first_name"].As<std::string>("");
    u.last_name     = body["last_name"].As<std::string>("");
    u.email         = body["email"].As<std::string>("");

    if (u.login.empty() || pw.empty() || u.first_name.empty() ||
        u.last_name.empty()) {
        throw ClientError(ErrorBody(
            "validation_failed",
            "login, password, first_name, last_name are required"));
    }

    u.password_hash = auth_.HashPassword(pw);
    u.id = users_.Get().NextUserId();
    if (!users_.Get().Create(u)) {
        throw ConflictError(ErrorBody(
            "login_taken", "login '" + u.login + "' is already taken"));
    }

    resp.SetStatus(userver::server::http::HttpStatus::kCreated);
    resp.SetHeader(std::string{"Location"}, "/v1/users?login=" + u.login);
    return userver::formats::json::ToString(dto::ToJson(u));
}

UserGet::UserGet(const userver::components::ComponentConfig& cfg,
                 const userver::components::ComponentContext& ctx)
    : userver::server::handlers::HttpHandlerBase(cfg, ctx),
      users_(ctx.FindComponent<repository::UserRepository>()),
      auth_(ctx.FindComponent<auth::AuthService>()) {}

std::string UserGet::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    req.GetHttpResponse().SetContentType("application/json");

    // Auth is enforced by the bearer checker (see static_config.yaml).
    const auto login = req.GetArg("login");
    const auto mask  = req.GetArg("nameMask");

    if (!login.empty()) {
        auto user = users_.Get().GetByLogin(login);
        if (!user) {
            throw ResourceNotFound(ErrorBody(
                "user_not_found", "user '" + login + "' not found"));
        }
        return userver::formats::json::ToString(dto::ToJson(*user));
    }

    if (!mask.empty()) {
        auto found = users_.Get().SearchByNameMask(mask);
        userver::formats::json::ValueBuilder list(
            userver::formats::common::Type::kArray);
        for (const auto& u : found) list.PushBack(dto::ToJson(u));
        userver::formats::json::ValueBuilder out;
        out["items"] = list.ExtractValue();
        out["count"] = static_cast<int64_t>(found.size());
        return userver::formats::json::ToString(out.ExtractValue());
    }

    throw ClientError(ErrorBody(
        "validation_failed",
        "one of 'login' or 'nameMask' query parameters is required"));
}

UserGetInternal::UserGetInternal(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : userver::server::handlers::HttpHandlerBase(cfg, ctx),
      users_(ctx.FindComponent<repository::UserRepository>()),
      auth_(ctx.FindComponent<auth::AuthService>()) {}

std::string UserGetInternal::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    req.GetHttpResponse().SetContentType("application/json");

    const auto& login = req.GetPathArg("login");
    auto user = users_.Get().GetByLogin(login);
    if (!user) {
        throw ResourceNotFound(ErrorBody(
            "user_not_found", "user '" + login + "' not found"));
    }
    return userver::formats::json::ToString(dto::ToJson(*user));
}

}  // namespace taxi::user::handlers
