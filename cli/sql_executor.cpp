#include "sql_executor.h"

#include <cstddef>
#include <ostream>
#include <utility>
#include <vector>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/result_writer.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_cli {

namespace {

using sqlite_manager::Cell;
using sqlite_manager::Connection;
using sqlite_manager::QueryResult;
using sqlite_manager::ResultWriter;
using sqlite_manager::Statement;
using sqlite_manager::ValueType;

// Steps a query to completion, collecting its columns and rows into the
// model, then renders it through `writer`. Each cell records its storage
// type and (for non-NULL values) its display text, so the writer can
// render faithfully. Returns exit code.
int RunQuery(Statement& stmt, const ResultWriter& writer, std::ostream& out,
             std::ostream& err) {
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

    writer.Write(result, out);
    return 0;
}

}  // namespace

int ExecuteSql(Connection& conn, const std::string& sql,
               const ResultWriter& writer, std::ostream& out,
               std::ostream& err) {
    // Statements run one at a time, in order, stopping at the first error
    // (as sqlite3_exec does). Each is prepared only after the previous one
    // has run, since it may depend on the schema that one changed.
    bool printed_rows = false;
    std::size_t pos = 0;
    while (true) {
        auto prepared = Statement::PrepareNext(conn, sql, pos);
        if (!prepared.ok()) {
            err << "Error: " << prepared.error().message << "\n";
            return 1;
        }
        Statement& stmt = prepared.value();
        if (!stmt.IsValid()) break;  // nothing left to run

        if (stmt.ColumnCount() > 0) {
            if (const int rc = RunQuery(stmt, writer, out, err); rc != 0) {
                return rc;
            }
            printed_rows = true;
        } else if (auto step = stmt.Step(); !step.ok()) {
            err << "Error: " << step.error().message << "\n";
            return 1;
        }
    }

    // Statements that produce no rows are otherwise silent: confirm
    // success once, unless a query already printed something.
    if (!printed_rows) out << "OK\n";
    return 0;
}

}  // namespace sqlite_manager_cli
