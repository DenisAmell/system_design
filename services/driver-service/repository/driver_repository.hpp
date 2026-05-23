#pragma once

#include <memory>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

#include "repository/i_driver_repository.hpp"

namespace taxi::driver::repository {

class DriverRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "driver-repository";

    DriverRepository(const userver::components::ComponentConfig&,
                     const userver::components::ComponentContext&);

    IDriverRepository& Get() noexcept { return *impl_; }
    const IDriverRepository& Get() const noexcept { return *impl_; }

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    std::unique_ptr<IDriverRepository> impl_;
};

}  // namespace taxi::driver::repository
