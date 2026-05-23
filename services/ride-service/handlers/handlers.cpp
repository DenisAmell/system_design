#include "handlers/handlers.hpp"

#include <cmath>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_status.hpp>

#include "auth/auth_checker.hpp"
#include "dto/ride_dto.hpp"

namespace taxi::ride::handlers {

namespace {

using userver::server::handlers::ClientError;
using userver::server::handlers::ConflictError;
using userver::server::handlers::ExceptionWithCode;
using userver::server::handlers::ExternalBody;
using userver::server::handlers::HandlerErrorCode;
using userver::server::handlers::ResourceNotFound;

class ForbiddenError final
    : public ExceptionWithCode<HandlerErrorCode::kForbidden> {
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

double EstimatePrice(const GeoPoint& a, const GeoPoint& b,
                     std::string_view car_class) {
    constexpr double kEarthKm = 6371.0;
    auto rad = [](double d) { return d * M_PI / 180.0; };
    const double dlat = rad(b.lat - a.lat);
    const double dlon = rad(b.lon - a.lon);
    const double h = std::sin(dlat / 2) * std::sin(dlat / 2) +
                     std::cos(rad(a.lat)) * std::cos(rad(b.lat)) *
                         std::sin(dlon / 2) * std::sin(dlon / 2);
    const double km = 2 * kEarthKm * std::asin(std::min(1.0, std::sqrt(h)));
    const double per_km = car_class == "business" ? 60.0
                       : car_class == "comfort"  ? 40.0
                                                 : 25.0;
    return 100.0 + km * per_km;
}

userver::formats::json::Value RideListJson(const std::vector<Ride>& rides) {
    userver::formats::json::ValueBuilder list(
        userver::formats::common::Type::kArray);
    for (const auto& r : rides) list.PushBack(dto::ToJson(r));
    userver::formats::json::ValueBuilder out;
    out["items"] = list.ExtractValue();
    out["count"] = static_cast<int64_t>(rides.size());
    return out.ExtractValue();
}

}  // namespace

#define TAXI_RIDE_CTOR(Name)                                                 \
    Name::Name(const userver::components::ComponentConfig& cfg,              \
               const userver::components::ComponentContext& ctx)             \
        : userver::server::handlers::HttpHandlerBase(cfg, ctx),              \
          rides_(ctx.FindComponent<repository::RideRepository>()),           \
          driver_client_(ctx.FindComponent<clients::DriverClient>()),        \
          user_client_(ctx.FindComponent<clients::UserClient>()) {}

TAXI_RIDE_CTOR(RideCreate)
TAXI_RIDE_CTOR(RideListActive)
TAXI_RIDE_CTOR(RideAccept)
TAXI_RIDE_CTOR(RideComplete)
TAXI_RIDE_CTOR(UserRidesHistory)

#undef TAXI_RIDE_CTOR


std::string RideCreate::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext& ctx) const {
    auto& resp = req.GetHttpResponse();
    resp.SetContentType("application/json");

    const auto& login = taxi::auth::GetAuthLogin(ctx);
    const auto body = ParseJsonOrThrow(req.RequestBody());

    Ride r;
    r.id              = rides_.Get().NextRideId();
    r.passenger_login = login;
    r.from.lat        = body["from"]["lat"].As<double>(0.0);
    r.from.lon        = body["from"]["lon"].As<double>(0.0);
    r.to.lat          = body["to"]["lat"].As<double>(0.0);
    r.to.lon          = body["to"]["lon"].As<double>(0.0);
    r.car_class       = body["car_class"].As<std::string>("economy");
    r.price           = EstimatePrice(r.from, r.to, r.car_class);
    r.status          = RideStatus::kCreated;

    rides_.Get().Create(r);

    resp.SetStatus(userver::server::http::HttpStatus::kCreated);
    resp.SetHeader(std::string{"Location"}, "/v1/rides/" + r.id);
    return userver::formats::json::ToString(dto::ToJson(r));
}


std::string RideListActive::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext& ctx) const {
    req.GetHttpResponse().SetContentType("application/json");

    const auto& login = taxi::auth::GetAuthLogin(ctx);

    if (req.GetArg("status") != "active") {
        throw ClientError(ErrorBody(
            "validation_failed", "only ?status=active is supported"));
    }

    if (!driver_client_.IsDriver(login)) {
        throw ForbiddenError(ErrorBody(
            "forbidden", "only registered drivers can list active rides"));
    }

    return userver::formats::json::ToString(
        RideListJson(rides_.Get().ListActive()));
}


std::string RideAccept::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext& ctx) const {
    req.GetHttpResponse().SetContentType("application/json");

    const auto& login = taxi::auth::GetAuthLogin(ctx);
    if (!driver_client_.IsDriver(login)) {
        throw ForbiddenError(ErrorBody(
            "forbidden", "only drivers can accept rides"));
    }

    const auto& ride_id = req.GetPathArg("id");
    if (!rides_.Get().Get(ride_id)) {
        throw ResourceNotFound(ErrorBody(
            "ride_not_found", "ride '" + ride_id + "' not found"));
    }
    if (!rides_.Get().TryAccept(ride_id, login)) {
        throw ConflictError(ErrorBody(
            "ride_not_available", "ride is already accepted or finished"));
    }

    driver_client_.SetStatus(login, "BUSY");

    auto updated = rides_.Get().Get(ride_id);
    return userver::formats::json::ToString(dto::ToJson(*updated));
}


std::string RideComplete::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext& ctx) const {
    req.GetHttpResponse().SetContentType("application/json");

    const auto& login = taxi::auth::GetAuthLogin(ctx);
    if (!driver_client_.IsDriver(login)) {
        throw ForbiddenError(ErrorBody(
            "forbidden", "only the assigned driver can complete the ride"));
    }

    const auto& ride_id = req.GetPathArg("id");
    if (!rides_.Get().Get(ride_id)) {
        throw ResourceNotFound(ErrorBody(
            "ride_not_found", "ride '" + ride_id + "' not found"));
    }
    if (!rides_.Get().TryComplete(ride_id, login)) {
        throw ConflictError(ErrorBody(
            "invalid_state",
            "ride must be ACCEPTED by the calling driver to complete"));
    }

    driver_client_.SetStatus(login, "FREE");

    auto updated = rides_.Get().Get(ride_id);
    return userver::formats::json::ToString(dto::ToJson(*updated));
}


std::string UserRidesHistory::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    req.GetHttpResponse().SetContentType("application/json");

    const auto& login = req.GetPathArg("login");
    if (!user_client_.Exists(login)) {
        throw ResourceNotFound(ErrorBody(
            "user_not_found", "user '" + login + "' not found"));
    }

    return userver::formats::json::ToString(
        RideListJson(rides_.Get().ListByUser(login)));
}

}  // namespace taxi::ride::handlers
