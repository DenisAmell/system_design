#pragma once

#include <memory>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

#include "repository/i_user_repository.hpp"

namespace taxi::user::repository {

class UserRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "user-repository";

    UserRepository(const userver::components::ComponentConfig&,
                   const userver::components::ComponentContext&);

    IUserRepository& Get() noexcept { return *impl_; }
    const IUserRepository& Get() const noexcept { return *impl_; }

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    std::unique_ptr<IUserRepository> impl_;
};

}  // namespace taxi::user::repository
