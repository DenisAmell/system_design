#include "handlers/handlers.hpp"

#include <userver/formats/json/serialize.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_status.hpp>

#include "auth/auth_checker.hpp"
#include "dto/driver_dto.hpp"

namespace taxi::driver::handlers {

namespace {

using userver::server::handlers::ClientError;
using userver::server::handlers::ConflictError;
using userver::server::handlers::ExternalBody;
using userver::server::handlers::ResourceNotFound;

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

constexpr std::string_view kDriverCacheKeyPrefix = "cache:driver:by_login:";
constexpr int kDriverCacheTtlSeconds = 30;

std::string DriverCacheKey(std::string_view login) {
    std::string key{kDriverCacheKeyPrefix};
    key.append(login);
    return key;
}

}  // namespace

#define TAXI_DRIVER_CTOR(Name)                                               \
    Name::Name(const userver::components::ComponentConfig& cfg,              \
               const userver::components::ComponentContext& ctx)             \
        : userver::server::handlers::HttpHandlerBase(cfg, ctx),              \
          drivers_(ctx.FindComponent<repository::DriverRepository>()),       \
          redis_(ctx.FindComponent<cache::RedisClient>()) {}

TAXI_DRIVER_CTOR(DriverRegister)
TAXI_DRIVER_CTOR(DriverGetInternal)
TAXI_DRIVER_CTOR(DriverSetStatusInternal)

#undef TAXI_DRIVER_CTOR

// ---------- POST /v1/drivers ----------
std::string DriverRegister::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext& ctx) const {
    auto& resp = req.GetHttpResponse();
    resp.SetContentType("application/json");

    const auto& caller_login = taxi::auth::GetAuthLogin(ctx);
    const auto body = ParseJsonOrThrow(req.RequestBody());

    Driver d;
    d.login      = body["login"].As<std::string>(caller_login);
    d.car_model  = body["car_model"].As<std::string>("");
    d.car_number = body["car_number"].As<std::string>("");
    d.car_class  = body["car_class"].As<std::string>("economy");

    if (d.car_model.empty() || d.car_number.empty()) {
        throw ClientError(ErrorBody(
            "validation_failed", "car_model and car_number are required"));
    }

    auto user_id = drivers_.Get().FindUserIdByLogin(d.login);
    if (!user_id) {
        throw ResourceNotFound(ErrorBody(
            "user_not_found", "user '" + d.login + "' not found"));
    }
    d.id      = drivers_.Get().NextDriverId();
    d.user_id = *user_id;

    if (!drivers_.Get().Create(d)) {
        throw ConflictError(ErrorBody(
            "driver_exists",
            "driver for '" + d.login + "' is already registered"));
    }
    drivers_.Get().PromoteUserToDriver(d.login);
    // Cache may hold a "this user is not a driver" 404 from a moments-ago
    // ride-service call. Drop it so the next IsDriver check sees the truth.
    redis_.Del(DriverCacheKey(d.login));

    resp.SetStatus(userver::server::http::HttpStatus::kCreated);
    resp.SetHeader(std::string{"Location"}, "/v1/drivers/" + d.id);
    return userver::formats::json::ToString(dto::ToJson(d));
}

// ---------- GET /internal/drivers/{login} ----------
// Hot path: hit by ride-service on every accept / complete / list-active.
// Cache-aside, short TTL because ride-service tolerates briefly-stale status.
std::string DriverGetInternal::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    auto& resp = req.GetHttpResponse();
    resp.SetContentType("application/json");

    const auto& login = req.GetPathArg("login");
    const auto cache_key = DriverCacheKey(login);
    if (auto cached = redis_.Get(cache_key); cached) {
        resp.SetHeader(std::string{"X-Cache"}, "HIT");
        return std::move(*cached);
    }
    auto driver = drivers_.Get().GetByLogin(login);
    if (!driver) {
        throw ResourceNotFound(ErrorBody(
            "driver_not_found", "driver '" + login + "' not found"));
    }
    const auto body = userver::formats::json::ToString(dto::ToJson(*driver));
    redis_.SetEx(cache_key, body, kDriverCacheTtlSeconds);
    resp.SetHeader(std::string{"X-Cache"}, "MISS");
    return body;
}

// ---------- POST /internal/drivers/{login}/status ----------
// Invalidates the by-login cache so ride-service's next read picks up the
// new BUSY / FREE / OFFLINE status immediately.
std::string DriverSetStatusInternal::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    req.GetHttpResponse().SetContentType("application/json");

    const auto& login = req.GetPathArg("login");
    const auto body = ParseJsonOrThrow(req.RequestBody());
    const auto status = body["status"].As<std::string>("");
    if (status != "FREE" && status != "BUSY" && status != "OFFLINE") {
        throw ClientError(ErrorBody(
            "validation_failed",
            "status must be one of FREE, BUSY, OFFLINE"));
    }
    if (!drivers_.Get().SetStatus(login, status)) {
        throw ResourceNotFound(ErrorBody(
            "driver_not_found", "driver '" + login + "' not found"));
    }
    redis_.Del(DriverCacheKey(login));

    return userver::formats::json::ToString(
        userver::formats::json::MakeObject("login", login, "status", status));
}

}  // namespace taxi::driver::handlers
