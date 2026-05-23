#pragma once

#include <string>
#include <string_view>

#include <userver/clients/http/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/schema.hpp>

namespace taxi::ride::clients {

class UserClient final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "user-client";

    UserClient(const userver::components::ComponentConfig&,
               const userver::components::ComponentContext&);

    bool Exists(std::string_view login) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    userver::clients::http::Client& http_;
    std::string base_url_;
    int timeout_ms_;
};

}  // namespace taxi::ride::clients
