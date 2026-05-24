#include "repository/postgres_driver_repository.hpp"

#include <mutex>

#include <userver/logging/log.hpp>

namespace taxi::driver::repository {

namespace {

constexpr const char* kCols =
    "id, user_id, login, car_model, car_number, car_class, status";

std::string ToString(std::string_view value) {
    return std::string(value.data(), value.size());
}

}  // namespace

PostgresDriverRepository::PostgresDriverRepository(std::string conninfo)
    : db_(std::move(conninfo)) {
    LOG_INFO() << "PostgresDriverRepository connected";
}

Driver PostgresDriverRepository::ReadDriver(
    const taxi::postgres::Result& result, int row) {
    Driver d;
    d.id = result.Get(row, 0);
    d.user_id = result.Get(row, 1);
    d.login = result.Get(row, 2);
    d.car_model = result.Get(row, 3);
    d.car_number = result.Get(row, 4);
    d.car_class = result.Get(row, 5);
    d.status = result.Get(row, 6);
    return d;
}

std::string PostgresDriverRepository::NextDriverId() {
    std::lock_guard lock(mu_);
    auto result = db_.Exec(R"(
        SELECT 'd-' || (
            COALESCE(MAX(substring(id FROM 3)::integer), 0) + 1
        )
        FROM drivers
        WHERE id ~ '^d-[0-9]+$';
    )");
    return result.Get(0, 0);
}

bool PostgresDriverRepository::Create(const Driver& d) {
    std::lock_guard lock(mu_);
    auto result = db_.ExecParams(R"(
        INSERT INTO drivers (id, user_id, login, car_model, car_number,
                             car_class, status)
        VALUES ($1, $2, $3, $4, $5, $6, $7)
        ON CONFLICT (login) DO NOTHING;
    )", {d.id, d.user_id, d.login, d.car_model, d.car_number, d.car_class,
         d.status});
    return result.RowsAffected() > 0;
}

std::optional<Driver> PostgresDriverRepository::GetByLogin(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols +
                            " FROM drivers WHERE login = $1 LIMIT 1;";
    auto result = db_.ExecParams(sql, {ToString(login)});
    if (result.Rows() == 0) return std::nullopt;
    return ReadDriver(result, 0);
}

std::optional<std::string> PostgresDriverRepository::FindUserIdByLogin(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    auto result = db_.ExecParams(
        "SELECT id FROM users WHERE login = $1 LIMIT 1;", {ToString(login)});
    if (result.Rows() == 0) return std::nullopt;
    return result.Get(0, 0);
}

bool PostgresDriverRepository::PromoteUserToDriver(std::string_view login) {
    std::lock_guard lock(mu_);
    auto result = db_.ExecParams(
        "UPDATE users SET role = 'driver' WHERE login = $1;",
        {ToString(login)});
    return result.RowsAffected() > 0;
}

bool PostgresDriverRepository::SetStatus(std::string_view login,
                                         std::string_view status) {
    std::lock_guard lock(mu_);
    auto result = db_.ExecParams(
        "UPDATE drivers SET status = $1 WHERE login = $2;",
        {ToString(status), ToString(login)});
    return result.RowsAffected() > 0;
}

}  // namespace taxi::driver::repository
