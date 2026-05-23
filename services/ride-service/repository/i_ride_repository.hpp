#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "models/ride.hpp"

namespace taxi::ride::repository {

class IRideRepository {
public:
    virtual ~IRideRepository() = default;

    virtual void Create(const Ride& r) = 0;
    virtual std::optional<Ride> Get(std::string_view id) const = 0;
    virtual std::vector<Ride> ListActive() const = 0;
    virtual std::vector<Ride> ListByUser(std::string_view login) const = 0;

    virtual bool TryAccept(std::string_view ride_id,
                           std::string_view driver_login) = 0;
    virtual bool TryComplete(std::string_view ride_id,
                             std::string_view driver_login) = 0;

    virtual std::string NextRideId() = 0;
};

}  // namespace taxi::ride::repository
