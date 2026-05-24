#include "repository/postgres_ride_repository.hpp"

#include <mutex>

#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>

namespace taxi::ride::repository {

namespace {

constexpr const char* kCols =
    "id, passenger_login, driver_login, from_lat, from_lon, to_lat, to_lon, "
    "car_class, status, price, created_at";

std::string ToString(std::string_view value) {
    return std::string(value.data(), value.size());
}

}  // namespace

PostgresRideRepository::PostgresRideRepository(std::string conninfo)
    : db_(std::move(conninfo)) {
    LOG_INFO() << "PostgresRideRepository connected";
}

Ride PostgresRideRepository::ReadRide(const taxi::postgres::Result& result,
                                      int row) {
    Ride r;
    r.id = result.Get(row, 0);
    r.passenger_login = result.Get(row, 1);
    if (!result.IsNull(row, 2)) {
        r.driver_login = result.Get(row, 2);
    }
    r.from.lat = std::stod(result.Get(row, 3));
    r.from.lon = std::stod(result.Get(row, 4));
    r.to.lat = std::stod(result.Get(row, 5));
    r.to.lon = std::stod(result.Get(row, 6));
    r.car_class = result.Get(row, 7);
    r.status = StatusFromString(result.Get(row, 8));
    r.price = std::stod(result.Get(row, 9));
    return r;
}

std::string PostgresRideRepository::NextRideId() {
    std::lock_guard lock(mu_);
    auto result = db_.Exec(R"(
        SELECT 'r-' || (
            COALESCE(MAX(substring(id FROM 3)::integer), 0) + 1
        )
        FROM rides
        WHERE id ~ '^r-[0-9]+$';
    )");
    return result.Get(0, 0);
}

void PostgresRideRepository::Create(const Ride& r) {
    std::lock_guard lock(mu_);
    const auto created = userver::utils::datetime::Timestring(
        r.created_at, "UTC", "%Y-%m-%dT%H:%M:%SZ");
    db_.ExecParams(R"(
        INSERT INTO rides (
            id, passenger_login, driver_login, from_lat, from_lon, to_lat,
            to_lon, car_class, status, price, created_at
        )
        VALUES ($1, $2, NULL, $3, $4, $5, $6, $7, $8, $9, $10);
    )", {r.id, r.passenger_login, std::to_string(r.from.lat),
         std::to_string(r.from.lon), std::to_string(r.to.lat),
         std::to_string(r.to.lon), r.car_class, taxi::ride::ToString(r.status),
         std::to_string(r.price), created});
}

std::optional<Ride> PostgresRideRepository::Get(std::string_view id) const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols +
                            " FROM rides WHERE id = $1 LIMIT 1;";
    auto result = db_.ExecParams(sql, {ToString(id)});
    if (result.Rows() == 0) return std::nullopt;
    return ReadRide(result, 0);
}

std::vector<Ride> PostgresRideRepository::ListActive() const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols + R"(
        FROM rides
        WHERE status = 'CREATED'
        ORDER BY created_at;
    )";
    auto result = db_.Exec(sql);
    std::vector<Ride> out;
    out.reserve(result.Rows());
    for (int row = 0; row < result.Rows(); ++row) {
        out.push_back(ReadRide(result, row));
    }
    return out;
}

std::vector<Ride> PostgresRideRepository::ListByUser(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols + R"(
        FROM rides
        WHERE passenger_login = $1 OR driver_login = $1
        ORDER BY created_at DESC;
    )";
    auto result = db_.ExecParams(sql, {ToString(login)});
    std::vector<Ride> out;
    out.reserve(result.Rows());
    for (int row = 0; row < result.Rows(); ++row) {
        out.push_back(ReadRide(result, row));
    }
    return out;
}

bool PostgresRideRepository::TryAccept(std::string_view ride_id,
                                       std::string_view driver_login) {
    std::lock_guard lock(mu_);
    auto result = db_.ExecParams(R"(
        UPDATE rides
        SET status = 'ACCEPTED', driver_login = $1
        WHERE id = $2 AND status = 'CREATED';
    )", {ToString(driver_login), ToString(ride_id)});
    return result.RowsAffected() > 0;
}

bool PostgresRideRepository::TryComplete(std::string_view ride_id,
                                         std::string_view driver_login) {
    std::lock_guard lock(mu_);
    auto result = db_.ExecParams(R"(
        UPDATE rides
        SET status = 'COMPLETED'
        WHERE id = $1 AND status = 'ACCEPTED' AND driver_login = $2;
    )", {ToString(ride_id), ToString(driver_login)});
    return result.RowsAffected() > 0;
}

}  // namespace taxi::ride::repository
