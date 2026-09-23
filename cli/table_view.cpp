#include "table_view.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <ostream>
#include <string>
#include <vector>

#include "sqlite_manager/query_result.h"

namespace sqlite_manager_cli {

namespace {

using sqlite_manager::Cell;
using sqlite_manager::QueryResult;
using sqlite_manager::ValueType;

// Control characters would break the frame (a newline splits the row) or
// drive the terminal (ESC sequences), so they are shown escaped:
// \n, \r, \t, or \xHH for the rest.
std::string EscapeControls(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const char ch : text) {
        const auto c = static_cast<unsigned char>(ch);
        if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else if (c < 0x20 || c == 0x7F) {
            std::array<char, 5> buf{};
            std::snprintf(buf.data(), buf.size(), "\\x%02x", c);
            out += buf.data();
        } else {
            out += ch;
        }
    }
    return out;
}

// How a cell is shown in a table: its escaped text, or "NULL" for SQL NULL.
std::string TableCell(const Cell& cell) {
    return cell.type == ValueType::kNull ? std::string("NULL")
                                         : EscapeControls(cell.text);
}

// Width of UTF-8 text in terminal columns, counted as code points (bytes
// that are not continuation bytes), so "Привет" is 6 wide, not 12. East
// Asian wide characters and emoji, which take two columns, are counted as
// one.
std::size_t DisplayWidth(const std::string& text) {
    return static_cast<std::size_t>(
        std::count_if(text.begin(), text.end(), [](char c) {
            return (static_cast<unsigned char>(c) & 0xC0U) != 0x80U;
        }));
}

}  // namespace

void TableView::Write(const QueryResult& result, std::ostream& out) const {
    const std::size_t columns = result.columns.size();

    std::vector<std::string> header(columns);
    for (std::size_t i = 0; i < columns; ++i) {
        header[i] = EscapeControls(result.columns[i]);
    }

    // Each column is as wide as the widest of its header and its cells.
    std::vector<std::size_t> width(columns, 0);
    for (std::size_t i = 0; i < columns; ++i) {
        width[i] = DisplayWidth(header[i]);
    }
    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            width[i] = std::max(width[i], DisplayWidth(TableCell(row[i])));
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
                << std::string(width[i] - DisplayWidth(cells[i]), ' ') << ' ';
        }
        out << "|\n";
    };

    frame();
    print_cells(header);
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
