#ifndef SQLITE_MANAGER_CONNECTION_H
#define SQLITE_MANAGER_CONNECTION_H

#include <string>

#include "sqlite_manager/result.h"

struct sqlite3;

namespace sqlite_manager {

class Connection final {
public:
    Connection() = default;
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    // Opens a database file, creating it if missing
    Status Open(const std::string& path);

    // Closes the connection. Safe to call when already closed (no-op)
    Status Close();

    bool IsOpen() const { return db_ != nullptr; }

    // Executes one or more SQL statements that produce no result rows
    Status Execute(const std::string& sql);

    // Escape hatch for layers that need the raw handle (Statement will)
    sqlite3* raw() const { return db_; }

private:
    sqlite3* db_ = nullptr;
};

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_CONNECTION_H
