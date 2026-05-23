#include "repository/sqlite_driver_repository.hpp"

#include <stdexcept>

#include <userver/logging/log.hpp>

namespace taxi::driver::repository {

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

Driver ReadDriver(sqlite3_stmt* s) {
    Driver d;
    d.id         = ColText(s, 0);
    d.user_id    = ColText(s, 1);
    d.login      = ColText(s, 2);
    d.car_model  = ColText(s, 3);
    d.car_number = ColText(s, 4);
    d.car_class  = ColText(s, 5);
    d.status     = ColText(s, 6);
    return d;
}

constexpr const char* kCols =
    "id, user_id, login, car_model, car_number, car_class, status";

}  // namespace

SqliteDriverRepository::SqliteDriverRepository(const std::string& db_path) {
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
    LOG_INFO() << "SqliteDriverRepository opened at " << db_path;
}

SqliteDriverRepository::~SqliteDriverRepository() {
    if (db_) sqlite3_close(db_);
}

void SqliteDriverRepository::InitSchema() {
    Exec(db_, R"(
        CREATE TABLE IF NOT EXISTS users (
            id            TEXT PRIMARY KEY,
            login         TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            first_name    TEXT NOT NULL,
            last_name     TEXT NOT NULL,
            email         TEXT,
            role          TEXT NOT NULL DEFAULT 'passenger'
        );
    )");
    Exec(db_, R"(
        CREATE TABLE IF NOT EXISTS drivers (
            id         TEXT PRIMARY KEY,
            user_id    TEXT NOT NULL,
            login      TEXT UNIQUE NOT NULL,
            car_model  TEXT NOT NULL,
            car_number TEXT NOT NULL,
            car_class  TEXT NOT NULL,
            status     TEXT NOT NULL DEFAULT 'OFFLINE',
            FOREIGN KEY (login) REFERENCES users(login)
        );
    )");
}

std::string SqliteDriverRepository::NextDriverId() {
    std::lock_guard lock(mu_);
    Stmt st(db_, "SELECT COUNT(*) FROM drivers;");
    sqlite3_step(st.get());
    const auto n = sqlite3_column_int64(st.get(), 0) + 1;
    return "d-" + std::to_string(n);
}

bool SqliteDriverRepository::Create(const Driver& d) {
    std::lock_guard lock(mu_);
    Stmt st(db_,
            "INSERT INTO drivers (id, user_id, login, car_model, car_number, "
            "car_class, status) VALUES (?, ?, ?, ?, ?, ?, ?);");
    BindText(st.get(), 1, d.id);
    BindText(st.get(), 2, d.user_id);
    BindText(st.get(), 3, d.login);
    BindText(st.get(), 4, d.car_model);
    BindText(st.get(), 5, d.car_number);
    BindText(st.get(), 6, d.car_class);
    BindText(st.get(), 7, d.status);
    const int rc = sqlite3_step(st.get());
    if (rc == SQLITE_DONE) return true;
    if (rc == SQLITE_CONSTRAINT) return false;
    throw std::runtime_error(std::string("Create driver failed: ") +
                             sqlite3_errmsg(db_));
}

std::optional<Driver> SqliteDriverRepository::GetByLogin(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols +
                            " FROM drivers WHERE login = ? LIMIT 1;";
    Stmt st(db_, sql.c_str());
    BindText(st.get(), 1, login);
    if (sqlite3_step(st.get()) != SQLITE_ROW) return std::nullopt;
    return ReadDriver(st.get());
}

std::optional<std::string> SqliteDriverRepository::FindUserIdByLogin(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    Stmt st(db_, "SELECT id FROM users WHERE login = ? LIMIT 1;");
    BindText(st.get(), 1, login);
    if (sqlite3_step(st.get()) != SQLITE_ROW) return std::nullopt;
    return ColText(st.get(), 0);
}

bool SqliteDriverRepository::PromoteUserToDriver(std::string_view login) {
    std::lock_guard lock(mu_);
    Stmt st(db_, "UPDATE users SET role = 'driver' WHERE login = ?;");
    BindText(st.get(), 1, login);
    sqlite3_step(st.get());
    return sqlite3_changes(db_) > 0;
}

bool SqliteDriverRepository::SetStatus(std::string_view login,
                                       std::string_view status) {
    std::lock_guard lock(mu_);
    Stmt st(db_, "UPDATE drivers SET status = ? WHERE login = ?;");
    BindText(st.get(), 1, status);
    BindText(st.get(), 2, login);
    sqlite3_step(st.get());
    return sqlite3_changes(db_) > 0;
}

}  // namespace taxi::driver::repository
