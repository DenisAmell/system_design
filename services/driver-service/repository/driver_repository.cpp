#include "repository/driver_repository.hpp"

#include <cstdlib>
#include <optional>
#include <stdexcept>

#include <userver/components/component_config.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "repository/postgres_driver_repository.hpp"
#include "repository/sqlite_driver_repository.hpp"

namespace taxi::driver::repository {

namespace {

std::optional<std::string> EnvValue(const char* env_name) {
    if (const char* value = std::getenv(env_name); value && *value) {
        return value;
    }
    return std::nullopt;
}

}  // namespace

DriverRepository::DriverRepository(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : userver::components::ComponentBase(cfg, ctx) {
    (void)ctx;
    const auto backend = EnvValue("TAXI_DRIVER_DB_BACKEND").value_or(
        EnvValue("TAXI_DB_BACKEND").value_or(
            cfg["backend"].As<std::string>("sqlite")));
    if (backend == "sqlite") {
        const auto path = EnvValue("TAXI_SQLITE_USERS_DB").value_or(
            cfg["sqlite"]["db_path"].As<std::string>("/data/users.db"));
        impl_ = std::make_unique<SqliteDriverRepository>(path);
    } else if (backend == "postgres") {
        const auto conninfo = EnvValue("TAXI_POSTGRES_DSN").value_or(
            cfg["postgres"]["dsn"].As<std::string>(
                "host=postgres port=5432 dbname=taxi user=taxi password=taxi"));
        impl_ = std::make_unique<PostgresDriverRepository>(conninfo);
    } else {
        throw std::runtime_error("unknown driver-repository backend: " +
                                 backend);
    }
    LOG_INFO() << "driver-repository initialised with backend=" << backend;
}

userver::yaml_config::Schema DriverRepository::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
type: object
description: Driver repository facade.
additionalProperties: false
properties:
    backend:
        type: string
        description: "Either 'sqlite' (default) or 'postgres'. Can be overridden by TAXI_DB_BACKEND."
        defaultDescription: sqlite
    sqlite:
        type: object
        description: SQLite backend options.
        additionalProperties: false
        properties:
            db_path:
                type: string
                description: Path to the SQLite database file (shared with user-service).
                defaultDescription: /data/users.db
    postgres:
        type: object
        description: PostgreSQL backend options.
        additionalProperties: false
        properties:
            dsn:
                type: string
                description: PostgreSQL libpq connection string. Can be overridden by TAXI_POSTGRES_DSN.
                defaultDescription: host=postgres port=5432 dbname=taxi user=taxi password=taxi
)");
}

}  // namespace taxi::driver::repository
