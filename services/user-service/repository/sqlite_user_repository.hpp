#pragma once

#include <string>
#include <string_view>

#include <sqlite3.h>

#include <userver/engine/mutex.hpp>

#include "repository/i_user_repository.hpp"

namespace taxi::user::repository {

class SqliteUserRepository final : public IUserRepository {
public:
    explicit SqliteUserRepository(const std::string& db_path);
    ~SqliteUserRepository() override;

    SqliteUserRepository(const SqliteUserRepository&) = delete;
    SqliteUserRepository& operator=(const SqliteUserRepository&) = delete;

    bool Create(const User& u) override;
    std::optional<User> GetByLogin(std::string_view login) const override;
    std::vector<User> SearchByNameMask(std::string_view mask) const override;
    bool PromoteToDriver(std::string_view login) override;
    std::string NextUserId() override;

private:
    void InitSchema();

    mutable userver::engine::Mutex mu_;
    sqlite3* db_{nullptr};
};

}  // namespace taxi::user::repository
