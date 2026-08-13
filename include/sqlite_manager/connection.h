#ifndef SQLITE_MANAGER_CONNECTION_H
#define SQLITE_MANAGER_CONNECTION_H

#include <cstdint>
#include <string>

#include "sqlite_manager/result.h"

struct sqlite3;

namespace sqlite_manager {

class Connection final {
public:
    // How Open() accesses the database file.
    enum class OpenMode {
        kReadWriteCreate,   // default: read/write, create if missing
        kReadWrite,         // read/write, fail if the file does not exist
        kReadOnly           // read only, fail if the file does not exist
    };

    Connection() = default;
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    // Opens a database file according to mode.
    // Use ":memory:" for a temporary in-memory database.
    // Fails with kMisuse if this connection is already open.
    Status Open(const std::string& path,
                OpenMode mode = OpenMode::kReadWriteCreate);

    // Closes the connection. Safe to call when already closed (no-op)
    Status Close();

    bool IsOpen() const { return db_ != nullptr; }

    // Executes one or more SQL statements that produce no result rows
    Status Execute(const std::string& sql);

    // Rowid of the most recent successful INSERT on this connection, or
    // 0 if no row has been inserted (or the connection is closed).
    std::int64_t LastInsertRowId() const;

    // Number of rows changed by the most recent INSERT, UPDATE or DELETE
    // on this connection, or 0 if none (or the connection is closed).
    std::int64_t Changes() const;

    // Sets how long a locked database is retried before an operation
    // gives up with kBusy. Zero (the default) disables the wait and
    // fails immediately. Fails with kMisuse if the connection is closed.
    Status BusyTimeout(int milliseconds);

    // Escape hatch for layers that need the raw handle (Statement will)
    sqlite3* raw() const { return db_; }

private:
    sqlite3* db_ = nullptr;
};

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_CONNECTION_H
