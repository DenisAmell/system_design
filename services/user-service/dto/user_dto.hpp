#pragma once

#include <string>
#include <string_view>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value.hpp>

#include "models/user.hpp"

namespace taxi::user::dto {

inline userver::formats::json::Value ToJson(const User& u) {
    return userver::formats::json::MakeObject(
        "id",         u.id,
        "login",      u.login,
        "first_name", u.first_name,
        "last_name",  u.last_name,
        "email",      u.email,
        "role",       ToString(u.role));
}

inline userver::formats::json::Value ErrorJson(std::string_view code,
                                               std::string_view message) {
    return userver::formats::json::MakeObject(
        "error", code, "message", message);
}

}  // namespace taxi::user::dto
