#include "repository/user_repository.hpp"

#include <stdexcept>

#include <userver/components/component_config.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "repository/sqlite_user_repository.hpp"

namespace taxi::user::repository {

UserRepository::UserRepository(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : userver::components::ComponentBase(cfg, ctx) {
    const auto backend = cfg["backend"].As<std::string>("sqlite");
    if (backend == "sqlite") {
        const auto path =
            cfg["sqlite"]["db_path"].As<std::string>("/data/users.db");
        impl_ = std::make_unique<SqliteUserRepository>(path);
    } else if (backend == "postgres") {
        // Stub: a future PostgresUserRepository would pull a
        // `userver::storages::postgres::Cluster` out of `ctx` here.
        throw std::runtime_error(
            "postgres backend is declared but not implemented yet — "
            "set backend: sqlite or add a PostgresUserRepository");
    } else {
        throw std::runtime_error("unknown user-repository backend: " +
                                 backend);
    }
    LOG_INFO() << "user-repository initialised with backend=" << backend;
}

userver::yaml_config::Schema UserRepository::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<
        userver::components::ComponentBase>(R"(
type: object
description: User repository facade. Selects the storage backend at startup.
additionalProperties: false
properties:
    backend:
        type: string
        description: "Either 'sqlite' (default) or 'postgres' (future)."
        defaultDescription: sqlite
    sqlite:
        type: object
        description: SQLite backend options.
        additionalProperties: false
        properties:
            db_path:
                type: string
                description: Path to the SQLite database file.
                defaultDescription: /data/users.db
    postgres:
        type: object
        description: PostgreSQL backend options (reserved for a future migration).
        additionalProperties: true
        properties: {}
)");
}

}  // namespace taxi::user::repository
