#ifndef SQLITE_MANAGER_CLI_SQL_EXECUTOR_H
#define SQLITE_MANAGER_CLI_SQL_EXECUTOR_H

#include <iosfwd>
#include <string>

namespace sqlite_manager {
class Connection;
class ResultWriter;
}  // namespace sqlite_manager

namespace sqlite_manager_cli {

// Executes an SQL string (one statement or a batch) against an open
// connection and reports the outcome. Statements run in order and stop at
// the first failure, which prints "Error: ..." to `err`. The rows of each
// query are collected into a QueryResult and handed to `writer` for
// rendering (so a batch of several queries renders several results). If
// no query printed anything, "OK" is printed once. Returns exit code
// (0 ok, 1 error).
//
// Shared by the single-shot mode and the REPL.
int ExecuteSql(sqlite_manager::Connection& conn, const std::string& sql,
               const sqlite_manager::ResultWriter& writer, std::ostream& out,
               std::ostream& err);

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_SQL_EXECUTOR_H
