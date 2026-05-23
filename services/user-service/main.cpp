#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "auth/auth_checker.hpp"
#include "auth/auth_service.hpp"
#include "handlers/handlers.hpp"
#include "repository/user_repository.hpp"

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory<
        taxi::auth::JwtCheckerFactory>();

    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::TestsuiteSupport>()
        .Append<taxi::auth::AuthService>()
        .Append<taxi::user::repository::UserRepository>()
        .Append<taxi::user::handlers::AuthLogin>()
        .Append<taxi::user::handlers::UserCreate>()
        .Append<taxi::user::handlers::UserGet>()
        .Append<taxi::user::handlers::UserGetInternal>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
