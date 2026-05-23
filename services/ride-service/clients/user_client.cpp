#include "clients/user_client.hpp"

#include <chrono>
#include <stdexcept>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/components/component_config.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace taxi::ride::clients {

UserClient::UserClient(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : userver::components::ComponentBase(cfg, ctx),
      http_(ctx.FindComponent<userver::components::HttpClient>()
                .GetHttpClient()),
      base_url_(cfg["base_url"].As<std::string>(
          "http://user-service:8080")),
      timeout_ms_(cfg["timeout_ms"].As<int>(2000)) {}

bool UserClient::Exists(std::string_view login) const {
    const auto url = base_url_ + "/internal/users/" + std::string{login};
    auto response = http_.CreateRequest()
                        .get(url)
                        .timeout(std::chrono::milliseconds{timeout_ms_})
                        .retry(2)
                        .perform();
    const auto status = static_cast<int>(response->status_code());
    if (status == 200) return true;
    if (status == 404) return false;
    LOG_WARNING() << "user-service returned " << status
                  << " for login=" << login;
    throw std::runtime_error("user-service unexpected status: " +
                             std::to_string(status));
}

userver::yaml_config::Schema UserClient::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
type: object
description: HTTP client to user-service (cluster-internal).
additionalProperties: false
properties:
    base_url:
        type: string
        description: user-service base URL (no trailing slash).
        defaultDescription: http://user-service:8080
    timeout_ms:
        type: integer
        description: Per-request timeout.
        defaultDescription: 2000
)");
}

}  // namespace taxi::ride::clients
