#pragma once

#include <string>
#include <string_view>

#include <userver/engine/mutex.hpp>

#include "postgres/connection.hpp"
#include "repository/i_user_repository.hpp"

namespace taxi::user::repository {

class PostgresUserRepository final : public IUserRepository {
public:
    explicit PostgresUserRepository(std::string conninfo);

    bool Create(const User& u) override;
    std::optional<User> GetByLogin(std::string_view login) const override;
    std::vector<User> SearchByNameMask(std::string_view mask) const override;
    bool PromoteToDriver(std::string_view login) override;
    std::string NextUserId() override;

private:
    static User ReadUser(const taxi::postgres::Result& result, int row);

    mutable userver::engine::Mutex mu_;
    mutable taxi::postgres::Connection db_;
};

}  // namespace taxi::user::repository
