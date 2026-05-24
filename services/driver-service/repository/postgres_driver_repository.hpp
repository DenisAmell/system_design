#pragma once

#include <string>
#include <string_view>

#include <userver/engine/mutex.hpp>

#include "postgres/connection.hpp"
#include "repository/i_driver_repository.hpp"

namespace taxi::driver::repository {

class PostgresDriverRepository final : public IDriverRepository {
public:
    explicit PostgresDriverRepository(std::string conninfo);

    bool Create(const Driver& d) override;
    std::optional<Driver> GetByLogin(std::string_view login) const override;
    std::optional<std::string> FindUserIdByLogin(
        std::string_view login) const override;
    bool PromoteUserToDriver(std::string_view login) override;
    bool SetStatus(std::string_view login, std::string_view status) override;
    std::string NextDriverId() override;

private:
    static Driver ReadDriver(const taxi::postgres::Result& result, int row);

    mutable userver::engine::Mutex mu_;
    mutable taxi::postgres::Connection db_;
};

}  // namespace taxi::driver::repository
