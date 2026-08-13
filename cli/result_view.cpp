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
std::string TableCell(const QueryResult::Cell& cell) {
    return cell.value_or("NULL");
}

// RFC 4180 field: quote it when it contains a comma, double quote, CR or
// LF; escape embedded quotes by doubling them.
std::string CsvField(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) {
        return value;
    }
    std::string out = "\"";
    for (const char c : value) {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
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

void CsvView::Render(const QueryResult& result, std::ostream& out) const {
    const std::size_t columns = result.columns.size();

    for (std::size_t i = 0; i < columns; ++i) {
        if (i > 0) out << ',';
        out << CsvField(result.columns[i]);
    }
    out << '\n';

    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < columns; ++i) {
            if (i > 0) out << ',';
            // SQL NULL becomes an empty field.
            out << CsvField(row[i].value_or(std::string()));
        }
        out << '\n';
    }
}

}  // namespace sqlite_manager_cli
