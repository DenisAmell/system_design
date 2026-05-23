#pragma once

#include <string_view>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value.hpp>

#include "models/driver.hpp"

namespace taxi::driver::dto {

inline userver::formats::json::Value ToJson(const Driver& d) {
    return userver::formats::json::MakeObject(
        "id",         d.id,
        "user_id",    d.user_id,
        "login",      d.login,
        "car_model",  d.car_model,
        "car_number", d.car_number,
        "car_class",  d.car_class,
        "status",     d.status);
}

inline userver::formats::json::Value ErrorJson(std::string_view code,
                                               std::string_view message) {
    return userver::formats::json::MakeObject(
        "error", code, "message", message);
}

}  // namespace taxi::driver::dto
