#pragma once

#include <string>
#include <string_view>

#include <userver/clients/http/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

namespace taxi::ride::clients {

class DriverClient final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "driver-client";

    DriverClient(const userver::components::ComponentConfig&,
                 const userver::components::ComponentContext&);

    bool IsDriver(std::string_view login) const;
    
    void SetStatus(std::string_view login, std::string_view status) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    userver::clients::http::Client& http_;
    std::string base_url_;
    int timeout_ms_;
};

}  // namespace taxi::ride::clients
