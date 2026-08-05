#include "sqlite_manager/connection.h"

#include <sqlite3.h>

#include <utility>

namespace sqlite_manager {

namespace {
// Captures the current error state of a connection into an Error.
Error MakeError(sqlite3* db) {
    return Error::FromSqlite(sqlite3_extended_errcode(db),
                             sqlite3_errmsg(db));
}
}  // namespace

Connection::~Connection() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
}

Connection::Connection(Connection&& other) noexcept
    : db_(std::exchange(other.db_, nullptr)) {}

Connection& Connection::operator=(Connection&& other) noexcept {
    if (this != &other) {
        if (db_ != nullptr) {
            sqlite3_close(db_);
        }
        db_ = std::exchange(other.db_, nullptr);
    }
    return *this;
}

Status Connection::Open(const std::string& path) {
    if (db_ != nullptr) {
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "connection is already open");
    }

    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(
        path.c_str(), &db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

    if (rc != SQLITE_OK) {
        Error error = (db != nullptr)
            ? MakeError(db)
            : Error::FromSqlite(rc, "unable to allocate database handle");
        sqlite3_close(db);
        return error;
    }

    db_ = db;
    return Ok();
}

Status Connection::Close() {
    if (db_ == nullptr) {
        return Ok();  // closing a closed connection is a no-op
    }

    const int rc = sqlite3_close(db_);
    if (rc != SQLITE_OK) {
        return MakeError(db_);
    }

    db_ = nullptr;
    return Ok();
}

Status Connection::Execute(const std::string& sql) {
    if (db_ == nullptr) {
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "connection is not open");
    }

    char* errmsg = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);

    if (rc != SQLITE_OK) {
        std::string message =
            (errmsg != nullptr) ? errmsg : "unknown error";
        sqlite3_free(errmsg);
        return Error::FromSqlite(rc, std::move(message));
    }

    return Ok();
}

}  // namespace sqlite_manager