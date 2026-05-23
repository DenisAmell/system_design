#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "clients/driver_client.hpp"
#include "clients/user_client.hpp"
#include "repository/ride_repository.hpp"

namespace taxi::ride::handlers {

#define TAXI_RIDE_HANDLER(ClassName, NameLiteral)                            \
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
        repository::RideRepository& rides_;                                  \
        const clients::DriverClient& driver_client_;                         \
        const clients::UserClient& user_client_;                             \
    };

TAXI_RIDE_HANDLER(RideCreate,        "handler-ride-create")
TAXI_RIDE_HANDLER(RideListActive,    "handler-ride-list-active")
TAXI_RIDE_HANDLER(RideAccept,        "handler-ride-accept")
TAXI_RIDE_HANDLER(RideComplete,      "handler-ride-complete")
TAXI_RIDE_HANDLER(UserRidesHistory,  "handler-user-rides")

#undef TAXI_RIDE_HANDLER

}  // namespace taxi::ride::handlers
