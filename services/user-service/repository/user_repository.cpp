#include "repository/user_repository.hpp"

#include <cstdlib>
#include <optional>
#include <stdexcept>

#include <userver/components/component_config.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "repository/mongo_user_repository.hpp"
#include "repository/postgres_user_repository.hpp"
#include "repository/sqlite_user_repository.hpp"

namespace taxi::user::repository {

namespace {

std::optional<std::string> EnvValue(const char* env_name) {
    if (const char* value = std::getenv(env_name); value && *value) {
        return value;
    }
    return std::nullopt;
}

}  // namespace

UserRepository::UserRepository(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : userver::components::ComponentBase(cfg, ctx) {
    (void)ctx;
    const auto backend = EnvValue("TAXI_USER_DB_BACKEND").value_or(
        EnvValue("TAXI_DB_BACKEND").value_or(
            cfg["backend"].As<std::string>("sqlite")));
    if (backend == "sqlite") {
        const auto path = EnvValue("TAXI_SQLITE_USERS_DB").value_or(
            cfg["sqlite"]["db_path"].As<std::string>("/data/users.db"));
        impl_ = std::make_unique<SqliteUserRepository>(path);
    } else if (backend == "postgres") {
        const auto conninfo = EnvValue("TAXI_POSTGRES_DSN").value_or(
            cfg["postgres"]["dsn"].As<std::string>(
                "host=postgres port=5432 dbname=taxi user=taxi password=taxi"));
        impl_ = std::make_unique<PostgresUserRepository>(conninfo);
    } else if (backend == "mongo") {
        const auto uri = EnvValue("TAXI_MONGO_URI").value_or(
            cfg["mongo"]["uri"].As<std::string>(
                "mongodb://taxi:taxi@mongo:27017/taxi?authSource=admin"));
        const auto db_name = EnvValue("TAXI_MONGO_DB").value_or(
            cfg["mongo"]["db_name"].As<std::string>("taxi"));
        impl_ = std::make_unique<MongoUserRepository>(uri, db_name);
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
        description: "Either 'sqlite' (default), 'postgres' or 'mongo'. Can be overridden by TAXI_USER_DB_BACKEND or TAXI_DB_BACKEND."
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
        description: PostgreSQL backend options.
        additionalProperties: false
        properties:
            dsn:
                type: string
                description: PostgreSQL libpq connection string. Can be overridden by TAXI_POSTGRES_DSN.
                defaultDescription: host=postgres port=5432 dbname=taxi user=taxi password=taxi
    mongo:
        type: object
        description: MongoDB backend options.
        additionalProperties: false
        properties:
            uri:
                type: string
                description: MongoDB connection URI. Can be overridden by TAXI_MONGO_URI.
                defaultDescription: mongodb://taxi:taxi@mongo:27017/taxi?authSource=admin
            db_name:
                type: string
                description: MongoDB database name. Can be overridden by TAXI_MONGO_DB.
                defaultDescription: taxi
)");
}

}  // namespace taxi::user::repository
