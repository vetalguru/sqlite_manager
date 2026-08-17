#include "gui/core/csv_importer.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "gui/core/row_editor.h"
#include "gui/core/schema_reader.h"
#include "sqlite_manager/connection.h"
#include "sqlite_manager/error.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/transaction.h"

namespace sqlite_manager_gui {

using sqlite_manager::Cell;
using sqlite_manager::Connection;
using sqlite_manager::Error;
using sqlite_manager::ErrorCode;
using sqlite_manager::Result;
using sqlite_manager::Transaction;
using sqlite_manager::ValueType;

std::vector<std::vector<std::string>> ParseCsv(const std::string& text) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool in_quotes = false;

    auto end_field = [&]() {
        row.push_back(std::move(field));
        field.clear();
    };
    auto end_row = [&]() {
        end_field();
        rows.push_back(std::move(row));
        row.clear();
    };

    const std::size_t n = text.size();
    for (std::size_t i = 0; i < n; ++i) {
        const char c = text[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < n && text[i + 1] == '"') {
                    field += '"';
                    ++i;  // skip the escaped quote
                } else {
                    in_quotes = false;
                }
            } else {
                field += c;
            }
        } else if (c == '"') {
            in_quotes = true;
        } else if (c == ',') {
            end_field();
        } else if (c == '\r') {
            // Ignore; CRLF is handled by the LF branch.
        } else if (c == '\n') {
            end_row();
        } else {
            field += c;
        }
    }

    // Flush a final row that did not end with a newline.
    if (in_quotes || !field.empty() || !row.empty()) {
        end_row();
    }
    return rows;
}

Result<int> ImportCsv(Connection& conn, const std::string& table,
                      const std::string& text, bool has_header) {
    const auto grid = ParseCsv(text);
    if (grid.empty()) return 0;

    std::vector<std::string> columns;
    std::size_t start = 0;
    if (has_header) {
        columns = grid.front();
        start = 1;
    } else {
        auto info = ReadTable(conn, table);
        if (!info.ok()) return info.error();
        for (const auto& column : info.value().columns) {
            columns.push_back(column.name);
        }
    }
    if (columns.empty()) {
        return Error(ErrorCode::kMisuse, 0, "no columns to import into");
    }

    auto txn = Transaction::Begin(conn);
    if (!txn.ok()) return txn.error();

    int imported = 0;
    for (std::size_t r = start; r < grid.size(); ++r) {
        const auto& fields = grid[r];
        std::vector<std::pair<std::string, Cell>> values;
        const std::size_t count =
            columns.size() < fields.size() ? columns.size() : fields.size();
        for (std::size_t c = 0; c < count; ++c) {
            Cell cell = fields[c].empty() ? Cell{ValueType::kNull, {}}
                                          : Cell{ValueType::kText, fields[c]};
            values.emplace_back(columns[c], std::move(cell));
        }
        if (auto inserted = InsertRow(conn, table, values); !inserted.ok()) {
            return inserted.error();  // txn rolls back on scope exit
        }
        ++imported;
    }

    if (auto committed = txn.value().Commit(); !committed.ok()) {
        return committed.error();
    }
    return imported;
}

}  // namespace sqlite_manager_gui
