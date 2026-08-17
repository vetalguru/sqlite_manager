#include "gui/core/query_runner.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_gui {

using sqlite_manager::Cell;
using sqlite_manager::Connection;
using sqlite_manager::QueryResult;
using sqlite_manager::Result;
using sqlite_manager::Statement;
using sqlite_manager::ValueType;

Result<QueryResult> RunSql(Connection& conn, const std::string& sql) {
    auto stmt = Statement::Prepare(conn, sql);
    if (!stmt.ok()) return stmt.error();

    Statement& s = stmt.value();
    const int columns = s.ColumnCount();

    QueryResult result;
    result.columns.reserve(static_cast<std::size_t>(columns));
    for (int i = 0; i < columns; ++i) {
        result.columns.push_back(s.ColumnName(i));
    }

    while (true) {
        auto step = s.Step();
        if (!step.ok()) return step.error();
        if (step.value() == Statement::StepResult::kDone) break;

        std::vector<Cell> row;
        row.reserve(static_cast<std::size_t>(columns));
        for (int i = 0; i < columns; ++i) {
            Cell cell;
            cell.type = s.ColumnType(i);
            if (cell.type != ValueType::kNull) {
                cell.text = s.ColumnText(i);
            }
            row.push_back(std::move(cell));
        }
        result.rows.push_back(std::move(row));
    }
    return result;
}

}  // namespace sqlite_manager_gui
