#include "gui/core/schema_editor.h"

#include <cstddef>
#include <string>
#include <utility>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/error.h"
#include "sqlite_manager/sql_util.h"

namespace sqlite_manager_gui {

using sqlite_manager::Connection;
using sqlite_manager::Error;
using sqlite_manager::ErrorCode;
using sqlite_manager::QuoteIdentifier;
using sqlite_manager::Status;

namespace {

Status Misuse(std::string message) {
    return Error(ErrorCode::kMisuse, 0, std::move(message));
}

}  // namespace

Status AddColumn(Connection& conn, const std::string& table,
                 const std::string& name, const std::string& type) {
    if (table.empty() || name.empty()) {
        return Misuse("table and column names are required");
    }
    std::string sql = "ALTER TABLE " + QuoteIdentifier(table) + " ADD COLUMN " +
                      QuoteIdentifier(name);
    if (!type.empty()) sql += " " + type;
    return conn.Execute(sql + ";");
}

Status DropColumn(Connection& conn, const std::string& table,
                  const std::string& name) {
    if (table.empty() || name.empty()) {
        return Misuse("table and column names are required");
    }
    return conn.Execute("ALTER TABLE " + QuoteIdentifier(table) +
                        " DROP COLUMN " + QuoteIdentifier(name) + ";");
}

Status CreateTable(Connection& conn, const std::string& name,
                   const std::vector<ColumnDef>& columns) {
    if (name.empty()) return Misuse("table name is required");
    if (columns.empty()) return Misuse("at least one column is required");

    std::string sql = "CREATE TABLE " + QuoteIdentifier(name) + " (";
    for (std::size_t i = 0; i < columns.size(); ++i) {
        const ColumnDef& column = columns[i];
        if (column.name.empty()) return Misuse("every column needs a name");
        if (i > 0) sql += ", ";
        sql += QuoteIdentifier(column.name);
        if (!column.type.empty()) sql += " " + column.type;
        if (column.primary_key) sql += " PRIMARY KEY";
        if (column.not_null) sql += " NOT NULL";
    }
    sql += ");";
    return conn.Execute(sql);
}

Status DropTable(Connection& conn, const std::string& name) {
    if (name.empty()) return Misuse("table name is required");
    return conn.Execute("DROP TABLE " + QuoteIdentifier(name) + ";");
}

}  // namespace sqlite_manager_gui
