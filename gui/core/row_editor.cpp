#include "gui/core/row_editor.h"

#include <cstddef>
#include <string>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/sql_util.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_gui {

using sqlite_manager::Cell;
using sqlite_manager::Connection;
using sqlite_manager::QuoteIdentifier;
using sqlite_manager::Result;
using sqlite_manager::Statement;
using sqlite_manager::Status;
using sqlite_manager::ValueType;

namespace {

// Binds a cell to a named parameter: NULL as SQL NULL, anything else as
// text (the column's affinity converts numeric text on storage).
Status BindCell(Statement& stmt, const std::string& param, const Cell& cell) {
    if (cell.type == ValueType::kNull) return stmt.BindNull(param);
    return stmt.BindText(param, cell.text);
}

// Runs a prepared, already-bound statement to completion.
Status RunToCompletion(Statement& stmt) {
    if (auto step = stmt.Step(); !step.ok()) return step.error();
    return sqlite_manager::Ok();
}

}  // namespace

Status UpdateCell(Connection& conn, const std::string& table,
                  std::int64_t rowid, const std::string& column,
                  const Cell& value) {
    const std::string sql = "UPDATE " + QuoteIdentifier(table) + " SET " +
                            QuoteIdentifier(column) +
                            " = :value WHERE rowid = :rowid;";
    auto stmt = Statement::Prepare(conn, sql);
    if (!stmt.ok()) return stmt.error();
    if (auto s = BindCell(stmt.value(), ":value", value); !s.ok()) return s;
    if (auto s = stmt.value().BindInt64(":rowid", rowid); !s.ok()) return s;
    return RunToCompletion(stmt.value());
}

Status DeleteRow(Connection& conn, const std::string& table,
                 std::int64_t rowid) {
    const std::string sql =
        "DELETE FROM " + QuoteIdentifier(table) + " WHERE rowid = :rowid;";
    auto stmt = Statement::Prepare(conn, sql);
    if (!stmt.ok()) return stmt.error();
    if (auto s = stmt.value().BindInt64(":rowid", rowid); !s.ok()) return s;
    return RunToCompletion(stmt.value());
}

Result<std::int64_t> InsertRow(
    Connection& conn, const std::string& table,
    const std::vector<std::pair<std::string, Cell>>& values) {
    std::string sql = "INSERT INTO " + QuoteIdentifier(table);
    if (values.empty()) {
        sql += " DEFAULT VALUES;";
    } else {
        std::string columns;
        std::string params;
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                columns += ", ";
                params += ", ";
            }
            columns += QuoteIdentifier(values[i].first);
            params += ":v" + std::to_string(i);
        }
        sql += " (" + columns + ") VALUES (" + params + ");";
    }

    auto stmt = Statement::Prepare(conn, sql);
    if (!stmt.ok()) return stmt.error();
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (auto s = BindCell(stmt.value(), ":v" + std::to_string(i),
                              values[i].second);
            !s.ok()) {
            return s.error();
        }
    }
    if (auto step = stmt.value().Step(); !step.ok()) return step.error();
    return conn.LastInsertRowId();
}

}  // namespace sqlite_manager_gui
