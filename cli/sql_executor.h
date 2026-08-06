#ifndef SQLITE_MANAGER_CLI_SQL_EXECUTOR_H
#define SQLITE_MANAGER_CLI_SQL_EXECUTOR_H

#include <iosfwd>
#include <string>

namespace sqlite_manager {
class Connection;
}

namespace sqlite_manager_cli {

// Executes one SQL string against an open connection and prints the
// outcome: result rows for queries, "OK" for statements without rows,
// "Error: ..." to `err` on failure. Returns exit code (0 ok, 1 error).
//
// Shared by the single-shot mode and the REPL.
int ExecuteSql(sqlite_manager::Connection& conn, const std::string& sql,
               std::ostream& out, std::ostream& err);

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_SQL_EXECUTOR_H