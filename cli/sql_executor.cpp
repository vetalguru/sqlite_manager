#include "sql_executor.h"

#include <algorithm>
#include <ostream>
#include <vector>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_cli {

namespace {

using sqlite_manager::Connection;
using sqlite_manager::Statement;

// Prints all rows of a prepared statement. Returns exit code.
int RunSelect(Statement& stmt, bool align,
              std::ostream& out, std::ostream& err) {
    const int columns = stmt.ColumnCount();

    std::vector<std::vector<std::string>> rows;
    while (true) {
        auto step = stmt.Step();
        if (!step.ok()) {
            err << "Error: " << step.error().message << "\n";
            return 1;
        }
        if (step.value() == Statement::StepResult::kDone) break;

        std::vector<std::string> row;
        row.reserve(static_cast<std::size_t>(columns));
        for (int i = 0; i < columns; ++i) {
            row.push_back(stmt.ColumnIsNull(i) ? "NULL"
                                               : stmt.ColumnText(i));
        }
        rows.push_back(std::move(row));
    }

    std::vector<std::size_t> width(static_cast<std::size_t>(columns), 0);
    if (align) {
        for (const auto& row : rows) {
            for (std::size_t i = 0; i < row.size(); ++i) {
                width[i] = std::max(width[i], row[i].size());
            }
        }
    }

    for (const auto& row : rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i > 0) out << " | ";
            out << row[i];
            if (align) {
                out << std::string(width[i] - row[i].size(), ' ');
            }
        }
        out << "\n";
    }
    return 0;
}

}  // namespace

int ExecuteSql(Connection& conn, const std::string& sql, bool align,
               std::ostream& out, std::ostream& err) {
    // Single statements go through Statement so result rows can be
    // printed; batches fail Prepare and fall back to Execute.
    auto stmt = Statement::Prepare(conn, sql);
    if (stmt.ok()) {
        if (stmt.value().ColumnCount() > 0) {
            return RunSelect(stmt.value(), align, out, err);
        }
        auto step = stmt.value().Step();
        if (!step.ok()) {
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
