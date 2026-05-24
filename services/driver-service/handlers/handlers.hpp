#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "cache/redis_client.hpp"
#include "repository/driver_repository.hpp"

namespace taxi::driver::handlers {

#define TAXI_DRIVER_HANDLER(ClassName, NameLiteral)                          \
    class ClassName final                                                    \
        : public userver::server::handlers::HttpHandlerBase {                \
    public:                                                                  \
        static constexpr std::string_view kName = NameLiteral;               \
        ClassName(const userver::components::ComponentConfig& cfg,           \
                  const userver::components::ComponentContext& ctx);         \
        std::string HandleRequestThrow(                                      \
            const userver::server::http::HttpRequest& req,                   \
            userver::server::request::RequestContext&) const override;       \
                                                                             \
    private:                                                                 \
        repository::DriverRepository& drivers_;                              \
        cache::RedisClient& redis_;                                          \
    };

TAXI_DRIVER_HANDLER(DriverRegister,           "handler-driver-register")
TAXI_DRIVER_HANDLER(DriverGetInternal,        "handler-driver-get-internal")
TAXI_DRIVER_HANDLER(DriverSetStatusInternal,  "handler-driver-set-status-internal")

#undef TAXI_DRIVER_HANDLER

}  // namespace taxi::driver::handlers
