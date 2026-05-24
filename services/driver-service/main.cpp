#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "auth/auth_checker.hpp"
#include "auth/auth_service.hpp"
#include "cache/redis_client.hpp"
#include "handlers/handlers.hpp"
#include "repository/driver_repository.hpp"

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory<
        taxi::auth::JwtCheckerFactory>();

    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::TestsuiteSupport>()
        .Append<taxi::auth::AuthService>()
        .Append<taxi::cache::RedisClient>()
        .Append<taxi::driver::repository::DriverRepository>()
        .Append<taxi::driver::handlers::DriverRegister>()
        .Append<taxi::driver::handlers::DriverGetInternal>()
        .Append<taxi::driver::handlers::DriverSetStatusInternal>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
