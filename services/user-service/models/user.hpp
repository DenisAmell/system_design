#pragma once

#include <string>

namespace taxi::user {

enum class Role { kPassenger, kDriver };

inline std::string ToString(Role r) {
    return r == Role::kDriver ? "driver" : "passenger";
}

inline Role RoleFromString(std::string_view s) {
    return s == "driver" ? Role::kDriver : Role::kPassenger;
}

struct User {
    std::string id;
    std::string login;
    std::string password_hash;
    std::string first_name;
    std::string last_name;
    std::string email;
    Role role{Role::kPassenger};
};

}  // namespace taxi::user
