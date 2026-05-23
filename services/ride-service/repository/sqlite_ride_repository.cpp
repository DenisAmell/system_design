#include "repository/sqlite_ride_repository.hpp"

#include <stdexcept>

#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>

namespace taxi::ride::repository {

namespace {

void Exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown sqlite error";
        sqlite3_free(err);
        throw std::runtime_error("sqlite exec failed: " + msg);
    }
}

struct Stmt {
    sqlite3_stmt* p{nullptr};
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &p, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("prepare failed: ") +
                                     sqlite3_errmsg(db));
        }
    }
    ~Stmt() { if (p) sqlite3_finalize(p); }
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    sqlite3_stmt* get() const { return p; }
};

void BindText(sqlite3_stmt* s, int i, std::string_view v) {
    sqlite3_bind_text(s, i, v.data(), static_cast<int>(v.size()),
                      SQLITE_TRANSIENT);
}

std::string ColText(sqlite3_stmt* s, int i) {
    const auto* p = reinterpret_cast<const char*>(sqlite3_column_text(s, i));
    return p ? std::string(p) : std::string{};
}

Ride ReadRide(sqlite3_stmt* s) {
    Ride r;
    r.id              = ColText(s, 0);
    r.passenger_login = ColText(s, 1);
    if (sqlite3_column_type(s, 2) != SQLITE_NULL) {
        r.driver_login = ColText(s, 2);
    }
    r.from.lat   = sqlite3_column_double(s, 3);
    r.from.lon   = sqlite3_column_double(s, 4);
    r.to.lat     = sqlite3_column_double(s, 5);
    r.to.lon     = sqlite3_column_double(s, 6);
    r.car_class  = ColText(s, 7);
    r.status     = StatusFromString(ColText(s, 8));
    r.price      = sqlite3_column_double(s, 9);
    return r;
}

constexpr const char* kCols =
    "id, passenger_login, driver_login, from_lat, from_lon, "
    "to_lat, to_lon, car_class, status, price, created_at";

}  // namespace

SqliteRideRepository::SqliteRideRepository(const std::string& db_path) {
    const int flags =
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(db_path.c_str(), &db_, flags, nullptr) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "open failed";
        throw std::runtime_error("cannot open sqlite db at " + db_path +
                                 ": " + msg);
    }
    Exec(db_, "PRAGMA foreign_keys = ON;");
    Exec(db_, "PRAGMA journal_mode = WAL;");
    InitSchema();
    LOG_INFO() << "SqliteRideRepository opened at " << db_path;
}

SqliteRideRepository::~SqliteRideRepository() {
    if (db_) sqlite3_close(db_);
}

void SqliteRideRepository::InitSchema() {
    Exec(db_, R"(
        CREATE TABLE IF NOT EXISTS rides (
            id              TEXT PRIMARY KEY,
            passenger_login TEXT NOT NULL,
            driver_login    TEXT,
            from_lat        REAL NOT NULL,
            from_lon        REAL NOT NULL,
            to_lat          REAL NOT NULL,
            to_lon          REAL NOT NULL,
            car_class       TEXT NOT NULL,
            status          TEXT NOT NULL DEFAULT 'CREATED',
            price           REAL NOT NULL,
            created_at      TEXT NOT NULL
        );
    )");
    Exec(db_, "CREATE INDEX IF NOT EXISTS rides_status_idx ON rides(status);");
    Exec(db_,
         "CREATE INDEX IF NOT EXISTS rides_passenger_idx "
         "ON rides(passenger_login);");
    Exec(db_,
         "CREATE INDEX IF NOT EXISTS rides_driver_idx ON rides(driver_login);");
}

std::string SqliteRideRepository::NextRideId() {
    std::lock_guard lock(mu_);
    Stmt st(db_, "SELECT COUNT(*) FROM rides;");
    sqlite3_step(st.get());
    const auto n = sqlite3_column_int64(st.get(), 0) + 1;
    return "r-" + std::to_string(n);
}

void SqliteRideRepository::Create(const Ride& r) {
    std::lock_guard lock(mu_);
    Stmt st(db_,
            "INSERT INTO rides (id, passenger_login, driver_login, from_lat, "
            "from_lon, to_lat, to_lon, car_class, status, price, created_at) "
            "VALUES (?, ?, NULL, ?, ?, ?, ?, ?, ?, ?, ?);");
    BindText(st.get(), 1, r.id);
    BindText(st.get(), 2, r.passenger_login);
    sqlite3_bind_double(st.get(), 3, r.from.lat);
    sqlite3_bind_double(st.get(), 4, r.from.lon);
    sqlite3_bind_double(st.get(), 5, r.to.lat);
    sqlite3_bind_double(st.get(), 6, r.to.lon);
    BindText(st.get(), 7, r.car_class);
    BindText(st.get(), 8, ToString(r.status));
    sqlite3_bind_double(st.get(), 9, r.price);
    const auto created = userver::utils::datetime::Timestring(
        r.created_at, "UTC", "%Y-%m-%dT%H:%M:%SZ");
    BindText(st.get(), 10, created);
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("Create ride failed: ") +
                                 sqlite3_errmsg(db_));
    }
}

std::optional<Ride> SqliteRideRepository::Get(std::string_view id) const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols +
                            " FROM rides WHERE id = ? LIMIT 1;";
    Stmt st(db_, sql.c_str());
    BindText(st.get(), 1, id);
    if (sqlite3_step(st.get()) != SQLITE_ROW) return std::nullopt;
    return ReadRide(st.get());
}

std::vector<Ride> SqliteRideRepository::ListActive() const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols +
                            " FROM rides WHERE status = 'CREATED' "
                            "ORDER BY created_at;";
    Stmt st(db_, sql.c_str());
    std::vector<Ride> out;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(ReadRide(st.get()));
    }
    return out;
}

std::vector<Ride> SqliteRideRepository::ListByUser(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    const std::string sql =
        std::string("SELECT ") + kCols +
        " FROM rides WHERE passenger_login = ? OR driver_login = ? "
        "ORDER BY created_at DESC;";
    Stmt st(db_, sql.c_str());
    BindText(st.get(), 1, login);
    BindText(st.get(), 2, login);
    std::vector<Ride> out;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(ReadRide(st.get()));
    }
    return out;
}

bool SqliteRideRepository::TryAccept(std::string_view ride_id,
                                     std::string_view driver_login) {
    std::lock_guard lock(mu_);
    Stmt st(db_,
            "UPDATE rides SET status = 'ACCEPTED', driver_login = ? "
            "WHERE id = ? AND status = 'CREATED';");
    BindText(st.get(), 1, driver_login);
    BindText(st.get(), 2, ride_id);
    sqlite3_step(st.get());
    return sqlite3_changes(db_) > 0;
}

bool SqliteRideRepository::TryComplete(std::string_view ride_id,
                                       std::string_view driver_login) {
    std::lock_guard lock(mu_);
    Stmt st(db_,
            "UPDATE rides SET status = 'COMPLETED' "
            "WHERE id = ? AND status = 'ACCEPTED' AND driver_login = ?;");
    BindText(st.get(), 1, ride_id);
    BindText(st.get(), 2, driver_login);
    sqlite3_step(st.get());
    return sqlite3_changes(db_) > 0;
}

}  // namespace taxi::ride::repository
