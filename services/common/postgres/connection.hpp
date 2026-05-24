#pragma once

#include <libpq-fe.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace taxi::postgres {

class Result final {
public:
    explicit Result(PGresult* result) : result_(result) {}
    ~Result() {
        if (result_) PQclear(result_);
    }

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    Result(Result&& other) noexcept : result_(other.result_) {
        other.result_ = nullptr;
    }

    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            if (result_) PQclear(result_);
            result_ = other.result_;
            other.result_ = nullptr;
        }
        return *this;
    }

    int Rows() const { return PQntuples(result_); }

    bool IsNull(int row, int col) const {
        return PQgetisnull(result_, row, col) != 0;
    }

    std::string Get(int row, int col) const {
        return IsNull(row, col) ? std::string{} : PQgetvalue(result_, row, col);
    }

    int RowsAffected() const {
        const char* value = PQcmdTuples(result_);
        return value && *value ? std::stoi(value) : 0;
    }

private:
    PGresult* result_{nullptr};
};

class Connection final {
public:
    explicit Connection(std::string conninfo) : conninfo_(std::move(conninfo)) {
        conn_ = PQconnectdb(conninfo_.c_str());
        if (PQstatus(conn_) != CONNECTION_OK) {
            const std::string msg = PQerrorMessage(conn_);
            PQfinish(conn_);
            conn_ = nullptr;
            throw std::runtime_error("postgres connection failed: " + msg);
        }
    }

    ~Connection() {
        if (conn_) PQfinish(conn_);
    }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Result Exec(std::string_view sql) {
        PGresult* result = PQexec(conn_, std::string(sql).c_str());
        Check(result);
        return Result(result);
    }

    Result ExecParams(
        std::string_view sql,
        const std::vector<std::optional<std::string>>& params = {}) {
        std::vector<const char*> values;
        values.reserve(params.size());
        for (const auto& value : params) {
            values.push_back(value ? value->c_str() : nullptr);
        }

        PGresult* result = PQexecParams(
            conn_, std::string(sql).c_str(), static_cast<int>(params.size()),
            nullptr, values.data(), nullptr, nullptr, 0);
        Check(result);
        return Result(result);
    }

private:
    void Check(PGresult* result) const {
        if (!result) {
            throw std::runtime_error("postgres query failed: " +
                                     std::string(PQerrorMessage(conn_)));
        }
        const auto status = PQresultStatus(result);
        if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
            const std::string msg = PQresultErrorMessage(result);
            PQclear(result);
            throw std::runtime_error("postgres query failed: " + msg);
        }
    }

    std::string conninfo_;
    PGconn* conn_{nullptr};
};

}  // namespace taxi::postgres
