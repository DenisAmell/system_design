#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "models/user.hpp"

namespace taxi::user::repository {

// Storage-agnostic interface. Today implemented by `SqliteUserRepository`;
// tomorrow a `PostgresUserRepository` can plug in without touching handlers.
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    // Returns false if `login` is already taken (UNIQUE constraint).
    virtual bool Create(const User& u) = 0;

    virtual std::optional<User> GetByLogin(std::string_view login) const = 0;

    virtual std::vector<User> SearchByNameMask(
        std::string_view mask) const = 0;

    // Used by driver-service via cross-service hook (or directly when the
    // shared users.db is mounted into both backends).
    virtual bool PromoteToDriver(std::string_view login) = 0;

    virtual std::string NextUserId() = 0;
};

}  // namespace taxi::user::repository
