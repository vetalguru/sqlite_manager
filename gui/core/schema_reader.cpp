#include "gui/core/schema_reader.h"

#include <string>
#include <utility>
#include <vector>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_gui {

using sqlite_manager::Connection;
using sqlite_manager::Result;
using sqlite_manager::Statement;

namespace {

ObjectKind KindFromType(const std::string& type) {
    if (type == "view") return ObjectKind::kView;
    if (type == "index") return ObjectKind::kIndex;
    if (type == "trigger") return ObjectKind::kTrigger;
    return ObjectKind::kTable;
}

}  // namespace

Result<std::vector<ObjectInfo>> ReadObjects(Connection& conn) {
    auto stmt = Statement::Prepare(conn,
                                   "SELECT type, name, sql FROM sqlite_master "
                                   "WHERE name NOT LIKE 'sqlite_%' "
                                   "ORDER BY type, name;");
    if (!stmt.ok()) return stmt.error();

    Statement& s = stmt.value();
    std::vector<ObjectInfo> objects;
    while (true) {
        auto step = s.Step();
        if (!step.ok()) return step.error();
        if (step.value() == Statement::StepResult::kDone) break;

        ObjectInfo info;
        info.kind = KindFromType(s.ColumnText(0));
        info.name = s.ColumnText(1);
        if (!s.ColumnIsNull(2)) info.sql = s.ColumnText(2);
        objects.push_back(std::move(info));
    }
    return objects;
}

Result<TableInfo> ReadTable(Connection& conn, const std::string& name) {
    TableInfo table;
    table.name = name;

    // Whether the object is a view (affects editability in the UI).
    auto type_stmt = Statement::Prepare(
        conn, "SELECT type FROM sqlite_master WHERE name = :name;");
    if (!type_stmt.ok()) return type_stmt.error();
    if (auto bound = type_stmt.value().BindText(":name", name); !bound.ok()) {
        return bound.error();
    }
    auto type_step = type_stmt.value().Step();
    if (!type_step.ok()) return type_step.error();
    if (type_step.value() == Statement::StepResult::kRow) {
        table.is_view = type_stmt.value().ColumnText(0) == "view";
    }

    // Columns, via the pragma_table_info table-valued function (which,
    // unlike the PRAGMA statement, accepts a bound parameter safely).
    auto stmt = Statement::Prepare(conn,
                                   "SELECT name, type, \"notnull\", "
                                   "dflt_value, pk FROM pragma_table_info("
                                   ":name);");
    if (!stmt.ok()) return stmt.error();
    if (auto bound = stmt.value().BindText(":name", name); !bound.ok()) {
        return bound.error();
    }

    Statement& s = stmt.value();
    while (true) {
        auto step = s.Step();
        if (!step.ok()) return step.error();
        if (step.value() == Statement::StepResult::kDone) break;

        ColumnInfo col;
        col.name = s.ColumnText(0);
        col.declared_type = s.ColumnText(1);
        col.not_null = s.ColumnInt64(2) != 0;
        if (!s.ColumnIsNull(3)) col.default_value = s.ColumnText(3);
        col.primary_key = s.ColumnInt64(4) != 0;
        table.columns.push_back(std::move(col));
    }
    return table;
}

}  // namespace sqlite_manager_gui
