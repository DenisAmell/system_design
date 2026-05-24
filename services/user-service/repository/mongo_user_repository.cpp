#include "repository/mongo_user_repository.hpp"

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <chrono>
#include <mutex>

#include <userver/logging/log.hpp>

namespace taxi::user::repository {

namespace {

std::string ToString(std::string_view value) {
    return std::string(value.data(), value.size());
}

std::string JsonEscape(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const char c : value) {
        if (c == '\\' || c == '"') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

std::string GetString(const bson_t* doc, const char* field) {
    bson_iter_t iter;
    if (bson_iter_init_find(&iter, doc, field) && BSON_ITER_HOLDS_UTF8(&iter)) {
        uint32_t len = 0;
        const char* value = bson_iter_utf8(&iter, &len);
        return std::string(value, len);
    }
    return {};
}

bson_t* NewJson(std::string_view json) {
    bson_error_t error;
    bson_t* doc = bson_new_from_json(
        reinterpret_cast<const uint8_t*>(json.data()),
        static_cast<ssize_t>(json.size()), &error);
    if (!doc) {
        throw std::runtime_error(std::string("invalid bson json: ") +
                                 error.message);
    }
    return doc;
}

}  // namespace

MongoUserRepository::MongoUserRepository(std::string uri, std::string db_name)
    : client_(std::move(uri), std::move(db_name)) {
    LOG_INFO() << "MongoUserRepository connected";
}

User MongoUserRepository::ReadUser(const bson_t* doc) {
    User u;
    u.id = GetString(doc, "id");
    u.login = GetString(doc, "login");
    u.password_hash = GetString(doc, "password_hash");
    u.first_name = GetString(doc, "first_name");
    u.last_name = GetString(doc, "last_name");
    u.email = GetString(doc, "email");
    u.role = RoleFromString(GetString(doc, "role"));
    return u;
}

std::string MongoUserRepository::NextUserId() {
    std::lock_guard lock(mu_);
    auto collection = client_.GetCollection("users");
    bson_t query;
    bson_init(&query);
    bson_error_t error;
    const auto count = mongoc_collection_count_documents(
        collection.Get(), &query, nullptr, nullptr, nullptr, &error);
    bson_destroy(&query);
    if (count < 0) {
        throw std::runtime_error(std::string("mongo count failed: ") +
                                 error.message);
    }
    return "u-" + std::to_string(count + 1);
}

bool MongoUserRepository::Create(const User& u) {
    std::lock_guard lock(mu_);
    auto collection = client_.GetCollection("users");

    bson_t doc;
    bson_init(&doc);
    BSON_APPEND_UTF8(&doc, "id", u.id.c_str());
    BSON_APPEND_UTF8(&doc, "login", u.login.c_str());
    BSON_APPEND_UTF8(&doc, "password_hash", u.password_hash.c_str());
    BSON_APPEND_UTF8(&doc, "first_name", u.first_name.c_str());
    BSON_APPEND_UTF8(&doc, "last_name", u.last_name.c_str());
    BSON_APPEND_UTF8(&doc, "email", u.email.c_str());
    const auto role = taxi::user::ToString(u.role);
    BSON_APPEND_UTF8(&doc, "role", role.c_str());
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
    BSON_APPEND_DATE_TIME(&doc, "created_at", now_ms);

    bson_error_t error;
    const bool ok = mongoc_collection_insert_one(
        collection.Get(), &doc, nullptr, nullptr, &error);
    bson_destroy(&doc);

    if (ok) return true;
    if (error.code == 11000) return false;
    throw std::runtime_error(std::string("mongo insert user failed: ") +
                             error.message);
}

std::optional<User> MongoUserRepository::GetByLogin(
    std::string_view login) const {
    std::lock_guard lock(mu_);
    auto collection = client_.GetCollection("users");

    bson_t query;
    bson_init(&query);
    BSON_APPEND_UTF8(&query, "login", ToString(login).c_str());

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(
        collection.Get(), &query, nullptr, nullptr);
    const bson_t* doc = nullptr;
    std::optional<User> out;
    if (mongoc_cursor_next(cursor, &doc)) {
        out = ReadUser(doc);
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(&query);
    return out;
}

std::vector<User> MongoUserRepository::SearchByNameMask(
    std::string_view mask) const {
    std::lock_guard lock(mu_);
    auto collection = client_.GetCollection("users");
    const auto escaped = JsonEscape(mask);
    const auto query_json =
        std::string(R"({"$or":[{"first_name":{"$regex":"^)") + escaped +
        R"(","$options":"i"}},{"last_name":{"$regex":"^)" + escaped +
        R"(","$options":"i"}}]})";
    bson_t* query = NewJson(query_json);
    bson_t* opts = NewJson(R"({"sort":{"login":1}})");

    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(
        collection.Get(), query, opts, nullptr);
    const bson_t* doc = nullptr;
    std::vector<User> out;
    while (mongoc_cursor_next(cursor, &doc)) {
        out.push_back(ReadUser(doc));
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    bson_destroy(opts);
    return out;
}

bool MongoUserRepository::PromoteToDriver(std::string_view login) {
    std::lock_guard lock(mu_);
    auto collection = client_.GetCollection("users");

    bson_t query;
    bson_init(&query);
    BSON_APPEND_UTF8(&query, "login", ToString(login).c_str());
    bson_t* update = NewJson(R"({"$set":{"role":"driver"}})");

    bson_error_t error;
    bson_t reply;
    const bool ok = mongoc_collection_update_one(
        collection.Get(), &query, update, nullptr, &reply, &error);
    taxi::mongo::ThrowIfError(ok, error, "mongo promote user");

    bson_iter_t iter;
    int64_t matched = 0;
    if (bson_iter_init_find(&iter, &reply, "matchedCount") &&
        BSON_ITER_HOLDS_INT64(&iter)) {
        matched = bson_iter_int64(&iter);
    }

    bson_destroy(&query);
    bson_destroy(update);
    bson_destroy(&reply);
    return matched > 0;
}

}  // namespace taxi::user::repository
