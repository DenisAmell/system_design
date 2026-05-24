#pragma once

#include <string>
#include <string_view>

#include <userver/engine/mutex.hpp>

#include "postgres/connection.hpp"
#include "repository/i_ride_repository.hpp"

namespace taxi::ride::repository {

class PostgresRideRepository final : public IRideRepository {
public:
    explicit PostgresRideRepository(std::string conninfo);

    void Create(const Ride& r) override;
    std::optional<Ride> Get(std::string_view id) const override;
    std::vector<Ride> ListActive() const override;
    std::vector<Ride> ListByUser(std::string_view login) const override;
    bool TryAccept(std::string_view ride_id,
                   std::string_view driver_login) override;
    bool TryComplete(std::string_view ride_id,
                     std::string_view driver_login) override;
    std::string NextRideId() override;

private:
    static Ride ReadRide(const taxi::postgres::Result& result, int row);

    mutable userver::engine::Mutex mu_;
    mutable taxi::postgres::Connection db_;
};

}  // namespace taxi::ride::repository
