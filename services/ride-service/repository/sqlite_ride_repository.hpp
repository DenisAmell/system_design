#pragma once

#include <string>
#include <string_view>

#include <sqlite3.h>

#include <userver/engine/mutex.hpp>

#include "repository/i_ride_repository.hpp"

namespace taxi::ride::repository {

class SqliteRideRepository final : public IRideRepository {
public:
    explicit SqliteRideRepository(const std::string& db_path);
    ~SqliteRideRepository() override;

    SqliteRideRepository(const SqliteRideRepository&) = delete;
    SqliteRideRepository& operator=(const SqliteRideRepository&) = delete;

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
    void InitSchema();

    mutable userver::engine::Mutex mu_;
    sqlite3* db_{nullptr};
};

}  // namespace taxi::ride::repository
