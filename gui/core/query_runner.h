#ifndef SQLITE_MANAGER_GUI_CORE_QUERY_RUNNER_H
#define SQLITE_MANAGER_GUI_CORE_QUERY_RUNNER_H

#include <string>

#include "sqlite_manager/query_result.h"
#include "sqlite_manager/result.h"

namespace sqlite_manager {
class Connection;
}  // namespace sqlite_manager

namespace sqlite_manager_gui {

// Runs a single SQL statement against an open connection and returns its
// rows as a QueryResult. A statement with result columns (SELECT, PRAGMA,
// ...) yields those rows; a statement without columns (INSERT/UPDATE/DDL)
// executes and yields an empty result. GTK-free and unit-tested.
sqlite_manager::Result<sqlite_manager::QueryResult> RunSql(
    sqlite_manager::Connection& conn, const std::string& sql);

}  // namespace sqlite_manager_gui

#endif  // SQLITE_MANAGER_GUI_CORE_QUERY_RUNNER_H
