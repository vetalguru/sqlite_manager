#include "sql_executor.h"

#include <cstddef>
#include <ostream>
#include <utility>
#include <vector>

#include "query_result.h"
#include "result_view.h"
#include "sqlite_manager/connection.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_cli {

namespace {

using sqlite_manager::Connection;
using sqlite_manager::Statement;
using sqlite_manager::ValueType;

// Steps a query to completion, collecting its columns and rows into the
// model, then renders it through `view`. Each cell records its storage
// type and (for non-NULL values) its display text, so the view can
// render faithfully. Returns exit code.
int RunQuery(Statement& stmt, const ResultView& view,
             std::ostream& out, std::ostream& err) {
    const int columns = stmt.ColumnCount();

    QueryResult result;
    result.columns.reserve(static_cast<std::size_t>(columns));
    for (int i = 0; i < columns; ++i) {
        result.columns.push_back(stmt.ColumnName(i));
    }

    while (true) {
        auto step = stmt.Step();
        if (!step.ok()) {
            err << "Error: " << step.error().message << "\n";
            return 1;
        }
        if (step.value() == Statement::StepResult::kDone) break;

        std::vector<Cell> row;
        row.reserve(static_cast<std::size_t>(columns));
        for (int i = 0; i < columns; ++i) {
            Cell cell;
            cell.type = stmt.ColumnType(i);
            if (cell.type != ValueType::kNull) {
                cell.text = stmt.ColumnText(i);
            }
            row.push_back(std::move(cell));
        }
        result.rows.push_back(std::move(row));
    }

    view.Render(result, out);
    return 0;
}

}  // namespace

int ExecuteSql(Connection& conn, const std::string& sql,
               const ResultView& view, std::ostream& out, std::ostream& err) {
    // Single statements go through Statement so result rows can be
    // rendered; batches fail Prepare and fall back to Execute.
    auto stmt = Statement::Prepare(conn, sql);
    if (stmt.ok()) {
        if (stmt.value().ColumnCount() > 0) {
            return RunQuery(stmt.value(), view, out, err);
        }
        if (auto step = stmt.value().Step(); !step.ok()) {
            err << "Error: " << step.error().message << "\n";
            return 1;
        }
        out << "OK\n";
        return 0;
    }

    if (auto s = conn.Execute(sql); !s.ok()) {
        err << "Error: " << s.error().message << "\n";
        return 1;
    }
    out << "OK\n";
    return 0;
}

}  // namespace sqlite_manager_cli
