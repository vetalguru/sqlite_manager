#include "result_view.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <ostream>
#include <string>
#include <vector>

#include "query_result.h"

namespace sqlite_manager_cli {

namespace {

// How a cell is shown in a table: its text, or "NULL" for SQL NULL.
std::string TableCell(const Cell& cell) {
    return cell.type == ValueType::kNull ? std::string("NULL") : cell.text;
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

// A quoted, escaped JSON string per RFC 8259.
std::string JsonString(const std::string& value) {
    std::string out = "\"";
    for (const char ch : value) {
        const auto c = static_cast<unsigned char>(ch);
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            default:
                if (c < 0x20) {
                    std::array<char, 7> buf{};
                    std::snprintf(buf.data(), buf.size(), "\\u%04x", c);
                    out += buf.data();
                } else {
                    out += ch;
                }
        }
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
            const Cell& cell = row[i];
            // SQL NULL becomes an empty field.
            out << CsvField(cell.type == ValueType::kNull ? std::string()
                                                          : cell.text);
        }
        out << '\n';
    }
}

void JsonView::Render(const QueryResult& result, std::ostream& out) const {
    const std::size_t columns = result.columns.size();

    out << '[';
    for (std::size_t r = 0; r < result.rows.size(); ++r) {
        out << (r == 0 ? "\n" : ",\n") << "  {";
        const auto& row = result.rows[r];
        for (std::size_t i = 0; i < columns; ++i) {
            if (i > 0) out << ", ";
            out << JsonString(result.columns[i]) << ": ";
            const Cell& cell = row[i];
            switch (cell.type) {
                case ValueType::kNull:
                    out << "null";
                    break;
                case ValueType::kInteger:
                case ValueType::kFloat:
                    // SQLite's text form of a number is a valid JSON number.
                    out << cell.text;
                    break;
                case ValueType::kText:
                case ValueType::kBlob:
                    out << JsonString(cell.text);
                    break;
            }
        }
        out << '}';
    }
    out << (result.rows.empty() ? "]\n" : "\n]\n");
}

}  // namespace sqlite_manager_cli
