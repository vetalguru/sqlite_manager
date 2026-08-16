#ifndef SQLITE_MANAGER_CLI_QUERY_RESULT_H
#define SQLITE_MANAGER_CLI_QUERY_RESULT_H

#include <string>
#include <vector>

#include "sqlite_manager/statement.h"

namespace sqlite_manager_cli {

using sqlite_manager::ValueType;

// Model: the tabular outcome of a query. Each cell carries its SQLite
// storage class plus a display-text form of the value (empty for NULL).
// The type lets a view render faithfully - e.g. JSON emits integers and
// reals as numbers, text as strings, NULL as null - while the text form
// serves table and CSV output directly.
struct Cell {
    ValueType type = ValueType::kNull;
    std::string text;  // display text of the value; empty when NULL
};

struct QueryResult {
    std::vector<std::string> columns;     // header names, left to right
    std::vector<std::vector<Cell>> rows;  // one inner vector per row
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_QUERY_RESULT_H
