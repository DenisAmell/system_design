#include "repository/postgres_user_repository.hpp"

#include <libpq-fe.h>

#include <mutex>

#include <userver/logging/log.hpp>

namespace taxi::user::repository {

namespace {

constexpr const char* kCols =
    "id, login, password_hash, first_name, last_name, email, role";

std::string ToString(std::string_view value) {
    return std::string(value.data(), value.size());
}

}  // namespace

PostgresUserRepository::PostgresUserRepository(std::string conninfo)
    : db_(std::move(conninfo)) {
    LOG_INFO() << "PostgresUserRepository connected";
}

User PostgresUserRepository::ReadUser(const taxi::postgres::Result& result,
                                      int row) {
    User u;
    u.id = result.Get(row, 0);
    u.login = result.Get(row, 1);
    u.password_hash = result.Get(row, 2);
    u.first_name = result.Get(row, 3);
    u.last_name = result.Get(row, 4);
    u.email = result.Get(row, 5);
    u.role = RoleFromString(result.Get(row, 6));
    return u;
}

std::string PostgresUserRepository::NextUserId() {
    std::lock_guard lock(mu_);
    auto result = db_.Exec(R"(
        SELECT 'u-' || (
            COALESCE(MAX(substring(id FROM 3)::integer), 0) + 1
        )
        FROM users
        WHERE id ~ '^u-[0-9]+$';
    )");
    return result.Get(0, 0);
}

bool PostgresUserRepository::Create(const User& u) {
    std::lock_guard lock(mu_);
    auto result = db_.ExecParams(R"(
        INSERT INTO users (id, login, password_hash, first_name, last_name,
                           email, role)
        VALUES ($1, $2, $3, $4, $5, $6, $7)
        ON CONFLICT (login) DO NOTHING;
    )", {u.id, u.login, u.password_hash, u.first_name, u.last_name, u.email,
         taxi::user::ToString(u.role)});
    return result.RowsAffected() > 0;
}

std::optional<User> PostgresUserRepository::GetByLogin(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols +
                            " FROM users WHERE login = $1 LIMIT 1;";
    auto result = db_.ExecParams(sql, {ToString(login)});
    if (result.Rows() == 0) return std::nullopt;
    return ReadUser(result, 0);
}

std::vector<User> PostgresUserRepository::SearchByNameMask(
    std::string_view mask) const {
    std::lock_guard lock(mu_);
    const auto pattern = ToString(mask) + "%";
    const std::string sql = std::string("SELECT ") + kCols + R"(
        FROM users
        WHERE first_name ILIKE $1 OR last_name ILIKE $1
        ORDER BY login;
    )";
    auto result = db_.ExecParams(sql, {pattern});
    std::vector<User> out;
    out.reserve(result.Rows());
    for (int row = 0; row < result.Rows(); ++row) {
        out.push_back(ReadUser(result, row));
    }
    return out;
}

bool PostgresUserRepository::PromoteToDriver(std::string_view login) {
    std::lock_guard lock(mu_);
    auto result = db_.ExecParams(
        "UPDATE users SET role = 'driver' WHERE login = $1;",
        {ToString(login)});
    return result.RowsAffected() > 0;
}

}  // namespace taxi::user::repository
