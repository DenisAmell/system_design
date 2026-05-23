#pragma once

#include <string>
#include <string_view>

#include <sqlite3.h>

#include <userver/engine/mutex.hpp>

#include "repository/i_driver_repository.hpp"

namespace taxi::driver::repository {

class SqliteDriverRepository final : public IDriverRepository {
public:
    explicit SqliteDriverRepository(const std::string& db_path);
    ~SqliteDriverRepository() override;

    SqliteDriverRepository(const SqliteDriverRepository&) = delete;
    SqliteDriverRepository& operator=(const SqliteDriverRepository&) = delete;

    bool Create(const Driver& d) override;
    std::optional<Driver> GetByLogin(std::string_view login) const override;
    std::optional<std::string> FindUserIdByLogin(
        std::string_view login) const override;
    bool PromoteUserToDriver(std::string_view login) override;
    bool SetStatus(std::string_view login,
                   std::string_view status) override;
    std::string NextDriverId() override;

private:
    void InitSchema();

    mutable userver::engine::Mutex mu_;
    sqlite3* db_{nullptr};
};

}  // namespace taxi::driver::repository
