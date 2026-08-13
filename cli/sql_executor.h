#ifndef SQLITE_MANAGER_CLI_SQL_EXECUTOR_H
#define SQLITE_MANAGER_CLI_SQL_EXECUTOR_H

#include <iosfwd>
#include <string>

namespace sqlite_manager {
class Connection;
}

namespace sqlite_manager_cli {

class ResultView;

// Executes one SQL string against an open connection and reports the
// outcome: query rows are collected into a QueryResult and handed to
// `view` for rendering; statements without rows print "OK"; failures
// print "Error: ..." to `err`. Returns exit code (0 ok, 1 error).
//
// Shared by the single-shot mode and the REPL.
int ExecuteSql(sqlite_manager::Connection& conn, const std::string& sql,
               const ResultView& view, std::ostream& out, std::ostream& err);

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_SQL_EXECUTOR_H
