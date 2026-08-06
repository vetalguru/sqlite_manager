#include "result_view.h"

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "query_result.h"

namespace sqlite_manager_cli {

namespace {

// How a SQL NULL cell is shown in a table.
std::string CellText(const QueryResult::Cell& cell) {
    return cell ? *cell : std::string("NULL");
}

}  // namespace

void TableView::Render(const QueryResult& result, std::ostream& out) const {
    const std::size_t columns = result.columns.size();

    // Each column is as wide as the widest of its header and its cells.
    std::vector<std::size_t> width(columns, 0);
    for (std::size_t i = 0; i < columns; ++i) {
        width[i] = result.columns[i].size();
    }
    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            width[i] = std::max(width[i], CellText(row[i]).size());
        }
    }

    // "| a | bb |" with every cell padded to its column width.
    auto print_cells = [&](const std::vector<std::string>& cells) {
        out << '|';
        for (std::size_t i = 0; i < columns; ++i) {
            out << ' ' << cells[i]
                << std::string(width[i] - cells[i].size(), ' ') << " |";
        }
        out << "\n";
    };

    print_cells(result.columns);

    // "|----|------|" rule between the header and the rows.
    out << '|';
    for (const std::size_t w : width) {
        out << std::string(w + 2, '-') << '|';
    }
    out << "\n";

    std::vector<std::string> cells(columns);
    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < columns; ++i) {
            cells[i] = CellText(row[i]);
        }
        print_cells(cells);
    }
}

}  // namespace sqlite_manager_cli
