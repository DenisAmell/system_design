#include "repository/sqlite_user_repository.hpp"

#include <stdexcept>

#include <userver/logging/log.hpp>

namespace taxi::user::repository {

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

User ReadUser(sqlite3_stmt* s) {
    User u;
    u.id            = ColText(s, 0);
    u.login         = ColText(s, 1);
    u.password_hash = ColText(s, 2);
    u.first_name    = ColText(s, 3);
    u.last_name     = ColText(s, 4);
    u.email         = ColText(s, 5);
    u.role          = RoleFromString(ColText(s, 6));
    return u;
}

constexpr const char* kCols =
    "id, login, password_hash, first_name, last_name, email, role";

}  // namespace

SqliteUserRepository::SqliteUserRepository(const std::string& db_path) {
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
    LOG_INFO() << "SqliteUserRepository opened at " << db_path;
}

SqliteUserRepository::~SqliteUserRepository() {
    if (db_) sqlite3_close(db_);
}

void SqliteUserRepository::InitSchema() {
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
}

std::string SqliteUserRepository::NextUserId() {
    std::lock_guard lock(mu_);
    Stmt st(db_, "SELECT COUNT(*) FROM users;");
    sqlite3_step(st.get());
    const auto n = sqlite3_column_int64(st.get(), 0) + 1;
    return "u-" + std::to_string(n);
}

bool SqliteUserRepository::Create(const User& u) {
    std::lock_guard lock(mu_);
    Stmt st(db_,
            "INSERT INTO users (id, login, password_hash, first_name, "
            "last_name, email, role) VALUES (?, ?, ?, ?, ?, ?, ?);");
    BindText(st.get(), 1, u.id);
    BindText(st.get(), 2, u.login);
    BindText(st.get(), 3, u.password_hash);
    BindText(st.get(), 4, u.first_name);
    BindText(st.get(), 5, u.last_name);
    BindText(st.get(), 6, u.email);
    BindText(st.get(), 7, ToString(u.role));
    const int rc = sqlite3_step(st.get());
    if (rc == SQLITE_DONE) return true;
    if (rc == SQLITE_CONSTRAINT) return false;
    throw std::runtime_error(std::string("Create user failed: ") +
                             sqlite3_errmsg(db_));
}

std::optional<User> SqliteUserRepository::GetByLogin(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    const std::string sql = std::string("SELECT ") + kCols +
                            " FROM users WHERE login = ? LIMIT 1;";
    Stmt st(db_, sql.c_str());
    BindText(st.get(), 1, login);
    if (sqlite3_step(st.get()) != SQLITE_ROW) return std::nullopt;
    return ReadUser(st.get());
}

std::vector<User> SqliteUserRepository::SearchByNameMask(
    std::string_view mask) const {
    std::lock_guard lock(mu_);
    const std::string pattern = std::string(mask) + "%";
    const std::string sql =
        std::string("SELECT ") + kCols +
        " FROM users WHERE first_name LIKE ? OR last_name LIKE ? "
        "ORDER BY login;";
    Stmt st(db_, sql.c_str());
    BindText(st.get(), 1, pattern);
    BindText(st.get(), 2, pattern);
    std::vector<User> out;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(ReadUser(st.get()));
    }
    return out;
}

bool SqliteUserRepository::PromoteToDriver(std::string_view login) {
    std::lock_guard lock(mu_);
    Stmt st(db_, "UPDATE users SET role = 'driver' WHERE login = ?;");
    BindText(st.get(), 1, login);
    sqlite3_step(st.get());
    return sqlite3_changes(db_) > 0;
}

}  // namespace taxi::user::repository
