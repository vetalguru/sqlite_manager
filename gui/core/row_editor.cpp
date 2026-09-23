#include "gui/core/row_editor.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

#include "gui/core/schema_reader.h"
#include "sqlite_manager/connection.h"
#include "sqlite_manager/error.h"
#include "sqlite_manager/sql_util.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_gui {

using sqlite_manager::Cell;
using sqlite_manager::Connection;
using sqlite_manager::Error;
using sqlite_manager::ErrorCode;
using sqlite_manager::QuoteIdentifier;
using sqlite_manager::Result;
using sqlite_manager::Statement;
using sqlite_manager::Status;
using sqlite_manager::ValueType;

namespace {

// SQLite matches identifiers case-insensitively (ASCII only).
bool SameIdentifier(const std::string& a, const std::string& b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) ==
                      std::tolower(static_cast<unsigned char>(y));
           });
}

// The first rowid alias that no column shadows, or nullptr when all
// three are taken. The aliases are fixed keywords: safe to use unquoted.
const char* FreeRowIdAlias(const std::vector<ColumnInfo>& columns) {
    for (const char* alias : {"rowid", "oid", "_rowid_"}) {
        const bool shadowed = std::any_of(
            columns.begin(), columns.end(), [alias](const ColumnInfo& c) {
                return SameIdentifier(c.name, alias);
            });
        if (!shadowed) return alias;
    }
    return nullptr;
}

Error RowIdHidden(const std::string& table) {
    return Error(ErrorCode::kError, 0,
                 "columns named rowid, oid and _rowid_ hide the rowid of " +
                     table);
}

// Runs a bound UPDATE/DELETE and requires it to touch exactly one row, so
// a stale or wrong rowid is reported instead of silently doing nothing.
Status RunSingleRowChange(Connection& conn, Statement& stmt,
                          std::int64_t rowid) {
    if (auto step = stmt.Step(); !step.ok()) return step.error();
    if (conn.Changes() != 1) {
        return Error(ErrorCode::kError, 0,
                     "no row with rowid " + std::to_string(rowid));
    }
    return sqlite_manager::Ok();
}

// Binds a cell to a named parameter: NULL as SQL NULL, anything else as
// text (the column's affinity converts numeric text on storage).
Status BindCell(Statement& stmt, const std::string& param, const Cell& cell) {
    if (cell.type == ValueType::kNull) return stmt.BindNull(param);
    return stmt.BindText(param, cell.text);
}

}  // namespace

Result<const char*> RowIdColumn(Connection& conn, const std::string& table) {
    auto info = ReadTable(conn, table);
    if (!info.ok()) return info.error();
    const char* id = FreeRowIdAlias(info.value().columns);
    if (id == nullptr) return RowIdHidden(table);
    return id;
}

Status UpdateCell(Connection& conn, const std::string& table,
                  std::int64_t rowid, const std::string& column,
                  const Cell& value) {
    auto info = ReadTable(conn, table);
    if (!info.ok()) return info.error();
    const char* id = FreeRowIdAlias(info.value().columns);
    if (id == nullptr) return RowIdHidden(table);
    const std::string sql = "UPDATE " + QuoteIdentifier(table) + " SET " +
                            QuoteIdentifier(column) + " = :value WHERE " + id +
                            " = :rowid;";
    auto stmt = Statement::Prepare(conn, sql);
    if (!stmt.ok()) return stmt.error();
    if (auto s = BindCell(stmt.value(), ":value", value); !s.ok()) return s;
    if (auto s = stmt.value().BindInt64(":rowid", rowid); !s.ok()) return s;
    return RunSingleRowChange(conn, stmt.value(), rowid);
}

Status DeleteRow(Connection& conn, const std::string& table,
                 std::int64_t rowid) {
    auto info = ReadTable(conn, table);
    if (!info.ok()) return info.error();
    const char* id = FreeRowIdAlias(info.value().columns);
    if (id == nullptr) return RowIdHidden(table);
    const std::string sql = "DELETE FROM " + QuoteIdentifier(table) +
                            " WHERE " + id + " = :rowid;";
    auto stmt = Statement::Prepare(conn, sql);
    if (!stmt.ok()) return stmt.error();
    if (auto s = stmt.value().BindInt64(":rowid", rowid); !s.ok()) return s;
    return RunSingleRowChange(conn, stmt.value(), rowid);
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
