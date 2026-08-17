#ifndef SQLITE_MANAGER_QUERY_RESULT_H
#define SQLITE_MANAGER_QUERY_RESULT_H

#include <string>
#include <vector>

#include "sqlite_manager/statement.h"  // ValueType

namespace sqlite_manager {

// The tabular outcome of a query. Each cell carries its SQLite storage
// class plus a display-text form of the value (empty for NULL). The type
// lets a writer render faithfully - e.g. JSON emits integers and reals as
// numbers, text as strings, NULL as null - while the text form serves
// table and CSV output directly.
struct Cell {
    ValueType type = ValueType::kNull;
    std::string text;  // display text of the value; empty when NULL
};

struct QueryResult {
    std::vector<std::string> columns;     // header names, left to right
    std::vector<std::vector<Cell>> rows;  // one inner vector per row
};

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_QUERY_RESULT_H
