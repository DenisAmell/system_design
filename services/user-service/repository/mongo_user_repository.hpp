#pragma once

#include <string>
#include <string_view>

#include <userver/engine/mutex.hpp>

#include "mongo/client.hpp"
#include "repository/i_user_repository.hpp"

namespace taxi::user::repository {

class MongoUserRepository final : public IUserRepository {
public:
    MongoUserRepository(std::string uri, std::string db_name);

    bool Create(const User& u) override;
    std::optional<User> GetByLogin(std::string_view login) const override;
    std::vector<User> SearchByNameMask(std::string_view mask) const override;
    bool PromoteToDriver(std::string_view login) override;
    std::string NextUserId() override;

private:
    static User ReadUser(const bson_t* doc);

    mutable userver::engine::Mutex mu_;
    mutable taxi::mongo::Client client_;
};

}  // namespace taxi::user::repository
