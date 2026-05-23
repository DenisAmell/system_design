#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "auth/auth_service.hpp"
#include "repository/user_repository.hpp"

namespace taxi::user::handlers {

#define TAXI_USER_HANDLER(ClassName, NameLiteral)                            \
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
        repository::UserRepository& users_;                                  \
        const auth::AuthService& auth_;                                      \
    };

TAXI_USER_HANDLER(AuthLogin,        "handler-auth-login")
TAXI_USER_HANDLER(UserCreate,       "handler-user-create")
TAXI_USER_HANDLER(UserGet,          "handler-user-get")
TAXI_USER_HANDLER(UserGetInternal,  "handler-user-get-internal")

#undef TAXI_USER_HANDLER

}  // namespace taxi::user::handlers
