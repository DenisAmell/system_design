#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/middlewares/pipeline_component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "auth/auth_checker.hpp"
#include "auth/auth_service.hpp"
#include "clients/driver_client.hpp"
#include "clients/user_client.hpp"
#include "handlers/handlers.hpp"
#include "repository/ride_repository.hpp"

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory<
        taxi::auth::JwtCheckerFactory>();

    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::TestsuiteSupport>()
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::HttpClientCore>()
        .Append<userver::clients::http::MiddlewarePipelineComponent>()
        .Append<userver::components::HttpClient>()
        .Append<taxi::auth::AuthService>()
        .Append<taxi::ride::repository::RideRepository>()
        .Append<taxi::ride::clients::DriverClient>()
        .Append<taxi::ride::clients::UserClient>()
        .Append<taxi::ride::handlers::RideCreate>()
        .Append<taxi::ride::handlers::RideListActive>()
        .Append<taxi::ride::handlers::RideAccept>()
        .Append<taxi::ride::handlers::RideComplete>()
        .Append<taxi::ride::handlers::UserRidesHistory>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
