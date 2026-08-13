#include "sqlite_manager/connection.h"

#include <sqlite3.h>

#include <utility>

namespace sqlite_manager {

namespace {
// Converts a Connection::OpenMode to the corresponding SQLite flags.
int ToSqliteFlags(Connection::OpenMode mode) {
    switch (mode) {
        case Connection::OpenMode::kReadWriteCreate:
            return SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        case Connection::OpenMode::kReadWrite:
            return SQLITE_OPEN_READWRITE;
        case Connection::OpenMode::kReadOnly:
            return SQLITE_OPEN_READONLY;
    }
    return SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;  // unreachable
}

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
Status Connection::Open(const std::string& path, OpenMode mode) {
    if (db_ != nullptr) {
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "connection is already open");
    }

    sqlite3* db = nullptr;
    const int rc = sqlite3_open_v2(path.c_str(), &db,
                                   ToSqliteFlags(mode), nullptr);
    if (rc != SQLITE_OK) {
        Error error = (db != nullptr)
            ? MakeError(db)
            : Error::FromSqlite(rc, sqlite3_errstr(rc));
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

    // Pass no errmsg out-param: on failure the connection holds the
    // error state, which MakeError() reads (extended code + message),
    // matching how Open()/Statement report errors.
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        return MakeError(db_);
    }

    return Ok();
}

std::int64_t Connection::LastInsertRowId() const {
    if (db_ == nullptr) return 0;
    return sqlite3_last_insert_rowid(db_);
}

std::int64_t Connection::Changes() const {
    if (db_ == nullptr) return 0;
    return sqlite3_changes64(db_);
}

Status Connection::BusyTimeout(int milliseconds) {
    if (db_ == nullptr) {
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "connection is not open");
    }
    const int rc = sqlite3_busy_timeout(db_, milliseconds);
    if (rc != SQLITE_OK) {
        return MakeError(db_);
    }
    return Ok();
}

}  // namespace sqlite_manager