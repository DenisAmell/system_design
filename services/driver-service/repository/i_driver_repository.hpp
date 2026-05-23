#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "models/driver.hpp"

namespace taxi::driver::repository {

class IDriverRepository {
public:
    virtual ~IDriverRepository() = default;

    virtual bool Create(const Driver& d) = 0;
    virtual std::optional<Driver> GetByLogin(std::string_view login) const = 0;

    virtual std::optional<std::string> FindUserIdByLogin(
        std::string_view login) const = 0;
    virtual bool PromoteUserToDriver(std::string_view login) = 0;

    virtual bool SetStatus(std::string_view login,
                           std::string_view status) = 0;

    virtual std::string NextDriverId() = 0;
};

}  // namespace taxi::driver::repository
