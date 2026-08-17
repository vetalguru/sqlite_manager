#include "table_view.h"

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "sqlite_manager/query_result.h"

namespace sqlite_manager_cli {

namespace {

using sqlite_manager::Cell;
using sqlite_manager::QueryResult;
using sqlite_manager::ValueType;

// How a cell is shown in a table: its text, or "NULL" for SQL NULL.
std::string TableCell(const Cell& cell) {
    return cell.type == ValueType::kNull ? std::string("NULL") : cell.text;
}

}  // namespace

void TableView::Write(const QueryResult& result, std::ostream& out) const {
    const std::size_t columns = result.columns.size();

    // Each column is as wide as the widest of its header and its cells.
    std::vector<std::size_t> width(columns, 0);
    for (std::size_t i = 0; i < columns; ++i) {
        width[i] = result.columns[i].size();
    }
    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            width[i] = std::max(width[i], TableCell(row[i]).size());
        }
    }

    // "+----+---------+"
    auto frame = [&]() {
        for (const std::size_t w : width) {
            out << '+' << std::string(w + 2, '-');
        }
        out << "+\n";
    };

    // "| 1  | M855    |"
    auto print_cells = [&](const std::vector<std::string>& cells) {
        for (std::size_t i = 0; i < columns; ++i) {
            out << "| " << cells[i]
                << std::string(width[i] - cells[i].size(), ' ') << ' ';
        }
        out << "|\n";
    };

    frame();
    print_cells(result.columns);
    frame();

    std::vector<std::string> cells(columns);
    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < columns; ++i) {
            cells[i] = TableCell(row[i]);
        }
        print_cells(cells);
    }
    frame();
}

}  // namespace sqlite_manager_cli
