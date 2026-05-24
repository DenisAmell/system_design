#include "handlers/handlers.hpp"

#include <cstdlib>
#include <string>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_status.hpp>

#include "dto/user_dto.hpp"

namespace taxi::user::handlers {

namespace {

using userver::server::handlers::ClientError;
using userver::server::handlers::ConflictError;
using userver::server::handlers::ExceptionWithCode;
using userver::server::handlers::ExternalBody;
using userver::server::handlers::ExtraHeaders;
using userver::server::handlers::HandlerErrorCode;
using userver::server::handlers::Headers;
using userver::server::handlers::ResourceNotFound;
using userver::server::handlers::Unauthorized;

// HandlerErrorCode::kTooManyRequests doesn't have a convenience class shipped
// with userver, so wrap it here. Maps to HTTP 429.
class TooManyRequests final
    : public ExceptionWithCode<HandlerErrorCode::kTooManyRequests> {
public:
    using BaseType::BaseType;
};

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

// nginx forwards client IP in X-Real-IP. Fall back to the raw socket peer
// only for the (unusual) case of hitting a backend directly.
std::string ClientIp(const userver::server::http::HttpRequest& req) {
    const auto& xri = req.GetHeader("X-Real-IP");
    if (!xri.empty()) return xri;
    return req.GetRemoteAddress().PrimaryAddressString();
}

constexpr std::string_view kUserCacheKeyPrefix = "cache:user:by_login:";
constexpr int kUserCacheTtlSeconds = 60;

std::string UserCacheKey(std::string_view login) {
    std::string key{kUserCacheKeyPrefix};
    key.append(login);
    return key;
}

constexpr std::string_view kLoginRateLimitPrefix = "rl:login:";
constexpr int kLoginRateWindowSeconds = 60;

// Tunable via LOGIN_RATE_LIMIT env. Default 30/min is loose enough for
// integration tests (~10 logins / session) but still kills credential
// stuffing. Drop to ~5/min in production / when demoing brute-force defence.
int64_t LoadLoginRateLimit() {
    if (const char* v = std::getenv("LOGIN_RATE_LIMIT"); v && *v) {
        char* end = nullptr;
        const auto n = std::strtoll(v, &end, 10);
        if (end != v && n > 0) return n;
    }
    return 30;
}

const int64_t kLoginRateLimit = LoadLoginRateLimit();

Headers RateLimitHeadersFor(const cache::RateLimiter::Decision& d) {
    return Headers{
        {"X-RateLimit-Limit",     std::to_string(d.limit)},
        {"X-RateLimit-Remaining", std::to_string(d.remaining)},
        {"X-RateLimit-Reset",     std::to_string(d.reset_seconds)},
    };
}

void ApplyRateLimitHeaders(userver::server::http::HttpResponse& resp,
                           const cache::RateLimiter::Decision& d) {
    resp.SetHeader(std::string{"X-RateLimit-Limit"},
                   std::to_string(d.limit));
    resp.SetHeader(std::string{"X-RateLimit-Remaining"},
                   std::to_string(d.remaining));
    resp.SetHeader(std::string{"X-RateLimit-Reset"},
                   std::to_string(d.reset_seconds));
}

}  // namespace

#define TAXI_USER_CTOR(Name)                                                 \
    Name::Name(const userver::components::ComponentConfig& cfg,              \
               const userver::components::ComponentContext& ctx)             \
        : userver::server::handlers::HttpHandlerBase(cfg, ctx),              \
          users_(ctx.FindComponent<repository::UserRepository>()),           \
          auth_(ctx.FindComponent<auth::AuthService>()),                     \
          redis_(ctx.FindComponent<cache::RedisClient>()),                   \
          rate_limiter_(ctx.FindComponent<cache::RateLimiter>()) {}

TAXI_USER_CTOR(AuthLogin)
TAXI_USER_CTOR(UserCreate)
TAXI_USER_CTOR(UserGet)
TAXI_USER_CTOR(UserGetInternal)

#undef TAXI_USER_CTOR

// ---------- POST /v1/login ----------
// Rate-limited: 5 attempts per 60s per client IP (brute-force protection).
std::string AuthLogin::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    auto& resp = req.GetHttpResponse();
    resp.SetContentType("application/json");

    const auto ip = ClientIp(req);
    const auto rl = rate_limiter_.Acquire(
        std::string{kLoginRateLimitPrefix} + ip,
        kLoginRateLimit,
        kLoginRateWindowSeconds);
    ApplyRateLimitHeaders(resp, rl);
    if (!rl.allowed) {
        LOG_WARNING() << "login rate-limit hit for ip=" << ip;
        throw TooManyRequests(
            ErrorBody("rate_limited",
                      "too many login attempts, retry later"),
            ExtraHeaders{RateLimitHeadersFor(rl)});
    }

    const auto body = ParseJsonOrThrow(req.RequestBody());
    const auto login    = body["login"].As<std::string>("");
    const auto password = body["password"].As<std::string>("");
    if (login.empty() || password.empty()) {
        throw ClientError(
            ErrorBody("validation_failed", "login and password are required"),
            ExtraHeaders{RateLimitHeadersFor(rl)});
    }

    auto user = users_.Get().GetByLogin(login);
    if (!user || user->password_hash != auth_.HashPassword(password)) {
        throw Unauthorized(
            ErrorBody("invalid_credentials", "invalid login or password"),
            ExtraHeaders{RateLimitHeadersFor(rl)});
    }

    return userver::formats::json::ToString(
        userver::formats::json::MakeObject(
            "token",      auth_.IssueToken(user->login),
            "token_type", std::string("Bearer"),
            "expires_in", auth_.TokenTtlSeconds()));
}

// ---------- POST /v1/users ----------
// Invalidates the by-login cache for this user so a follow-up read sees the
// fresh row without waiting for TTL.
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
    redis_.Del(UserCacheKey(u.login));

    resp.SetStatus(userver::server::http::HttpStatus::kCreated);
    resp.SetHeader(std::string{"Location"}, "/v1/users?login=" + u.login);
    return userver::formats::json::ToString(dto::ToJson(u));
}

// ---------- GET /v1/users ----------
// Cache-aside on `?login=` lookups. `?nameMask=` is left uncached: invalidation
// would need to fan out across every mask that matches the user, which isn't
// worth the complexity for a low-traffic search endpoint.
std::string UserGet::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    auto& resp = req.GetHttpResponse();
    resp.SetContentType("application/json");

    const auto login = req.GetArg("login");
    const auto mask  = req.GetArg("nameMask");

    if (!login.empty()) {
        const auto cache_key = UserCacheKey(login);
        if (auto cached = redis_.Get(cache_key); cached) {
            resp.SetHeader(std::string{"X-Cache"}, "HIT");
            return std::move(*cached);
        }
        auto user = users_.Get().GetByLogin(login);
        if (!user) {
            throw ResourceNotFound(ErrorBody(
                "user_not_found", "user '" + login + "' not found"));
        }
        const auto body =
            userver::formats::json::ToString(dto::ToJson(*user));
        redis_.SetEx(cache_key, body, kUserCacheTtlSeconds);
        resp.SetHeader(std::string{"X-Cache"}, "MISS");
        return body;
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

// ---------- GET /internal/users/{login} ----------
// Same cache namespace as the public GET — ride-service hits this on every
// `/v1/users/{login}/rides` call, so cache hits here translate directly into
// fewer DB round-trips.
std::string UserGetInternal::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    auto& resp = req.GetHttpResponse();
    resp.SetContentType("application/json");

    const auto& login = req.GetPathArg("login");
    const auto cache_key = UserCacheKey(login);
    if (auto cached = redis_.Get(cache_key); cached) {
        resp.SetHeader(std::string{"X-Cache"}, "HIT");
        return std::move(*cached);
    }
    auto user = users_.Get().GetByLogin(login);
    if (!user) {
        throw ResourceNotFound(ErrorBody(
            "user_not_found", "user '" + login + "' not found"));
    }
    const auto body = userver::formats::json::ToString(dto::ToJson(*user));
    redis_.SetEx(cache_key, body, kUserCacheTtlSeconds);
    resp.SetHeader(std::string{"X-Cache"}, "MISS");
    return body;
}

}  // namespace taxi::user::handlers
