#pragma once

#include <string_view>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/datetime.hpp>

#include "models/ride.hpp"

namespace taxi::ride::dto {

inline userver::formats::json::Value ToJson(const GeoPoint& g) {
    return userver::formats::json::MakeObject("lat", g.lat, "lon", g.lon);
}

inline userver::formats::json::Value ToJson(const Ride& r) {
    userver::formats::json::ValueBuilder b;
    b["id"] = r.id;
    b["passenger_login"] = r.passenger_login;
    if (r.driver_login) {
        b["driver_login"] = *r.driver_login;
    } else {
        b["driver_login"] = userver::formats::json::Value();
    }
    b["from"]       = ToJson(r.from);
    b["to"]         = ToJson(r.to);
    b["car_class"]  = r.car_class;
    b["status"]     = ToString(r.status);
    b["price"]      = r.price;
    b["created_at"] = userver::utils::datetime::Timestring(
        r.created_at, "UTC", "%Y-%m-%dT%H:%M:%SZ");
    return b.ExtractValue();
}

inline userver::formats::json::Value ErrorJson(std::string_view code,
                                               std::string_view message) {
    return userver::formats::json::MakeObject(
        "error", code, "message", message);
}

}  // namespace taxi::ride::dto
