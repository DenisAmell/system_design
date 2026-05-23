#pragma once

#include <memory>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

#include "repository/i_ride_repository.hpp"

namespace taxi::ride::repository {

class RideRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "ride-repository";

    RideRepository(const userver::components::ComponentConfig&,
                   const userver::components::ComponentContext&);

    IRideRepository& Get() noexcept { return *impl_; }
    const IRideRepository& Get() const noexcept { return *impl_; }

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    std::unique_ptr<IRideRepository> impl_;
};

}  // namespace taxi::ride::repository
