#pragma once

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace taxi::mongo {

class DriverRuntime final {
public:
    DriverRuntime() { mongoc_init(); }
    ~DriverRuntime() { mongoc_cleanup(); }

    DriverRuntime(const DriverRuntime&) = delete;
    DriverRuntime& operator=(const DriverRuntime&) = delete;
};

inline DriverRuntime& Runtime() {
    static DriverRuntime runtime;
    return runtime;
}

class Collection final {
public:
    explicit Collection(mongoc_collection_t* collection)
        : collection_(collection) {}

    ~Collection() {
        if (collection_) mongoc_collection_destroy(collection_);
    }

    Collection(const Collection&) = delete;
    Collection& operator=(const Collection&) = delete;

    mongoc_collection_t* Get() const { return collection_; }

private:
    mongoc_collection_t* collection_{nullptr};
};

class Client final {
public:
    Client(std::string uri, std::string db_name)
        : uri_(std::move(uri)), db_name_(std::move(db_name)) {
        Runtime();
        client_ = mongoc_client_new(uri_.c_str());
        if (!client_) {
            throw std::runtime_error("cannot create mongo client for " + uri_);
        }
    }

    ~Client() {
        if (client_) mongoc_client_destroy(client_);
    }

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Collection GetCollection(std::string_view name) const {
        return Collection(mongoc_client_get_collection(
            client_, db_name_.c_str(), std::string(name).c_str()));
    }

private:
    std::string uri_;
    std::string db_name_;
    mongoc_client_t* client_{nullptr};
};

inline void ThrowIfError(bool ok, const bson_error_t& error,
                         std::string_view action) {
    if (!ok) {
        throw std::runtime_error(std::string(action) + " failed: " +
                                 error.message);
    }
}

}  // namespace taxi::mongo
