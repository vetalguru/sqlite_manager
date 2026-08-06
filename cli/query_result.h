#ifndef SQLITE_MANAGER_CLI_QUERY_RESULT_H
#define SQLITE_MANAGER_CLI_QUERY_RESULT_H

#include <optional>
#include <string>
#include <vector>

namespace sqlite_manager_cli {

// Model: the tabular outcome of a query, as display strings. It holds
// data only - no presentation logic; a ResultView decides how to render
// it. A cell that is SQL NULL is stored as std::nullopt so the view
// controls how NULL appears ("NULL" in a table, null in JSON, etc.).
struct QueryResult {
    using Cell = std::optional<std::string>;

    std::vector<std::string> columns;       // header names, left to right
    std::vector<std::vector<Cell>> rows;    // one inner vector per row
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_QUERY_RESULT_H
