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

// Prints a prepared query as an aligned table with a header, e.g.
//   | id | name    |
//   |----|---------|
//   | 1  | M855    |
// Returns exit code.
int RunSelect(Statement& stmt, std::ostream& out, std::ostream& err) {
    const int columns = stmt.ColumnCount();

    std::vector<std::string> header;
    header.reserve(static_cast<std::size_t>(columns));
    for (int i = 0; i < columns; ++i) {
        header.push_back(stmt.ColumnName(i));
    }

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

    // Each column is as wide as the widest of its header and its cells.
    std::vector<std::size_t> width(static_cast<std::size_t>(columns), 0);
    for (std::size_t i = 0; i < header.size(); ++i) {
        width[i] = header[i].size();
    }
    for (const auto& row : rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            width[i] = std::max(width[i], row[i].size());
        }
    }

    // "| a | bb |" with every cell padded to its column width.
    auto print_row = [&](const std::vector<std::string>& cells) {
        out << '|';
        for (std::size_t i = 0; i < cells.size(); ++i) {
            out << ' ' << cells[i]
                << std::string(width[i] - cells[i].size(), ' ') << " |";
        }
        out << "\n";
    };

    print_row(header);

    // "|----|------|" rule between the header and the rows.
    out << '|';
    for (const std::size_t w : width) {
        out << std::string(w + 2, '-') << '|';
    }
    out << "\n";

    for (const auto& row : rows) {
        print_row(row);
    }
    return 0;
}

}  // namespace

int ExecuteSql(Connection& conn, const std::string& sql,
               std::ostream& out, std::ostream& err) {
    // Single statements go through Statement so result rows can be
    // printed; batches fail Prepare and fall back to Execute.
    auto stmt = Statement::Prepare(conn, sql);
    if (stmt.ok()) {
        if (stmt.value().ColumnCount() > 0) {
            return RunSelect(stmt.value(), out, err);
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
