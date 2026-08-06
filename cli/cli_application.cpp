#include "cli_application.h"

#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

#include "arg_parser.h"
#include "sqlite_manager/connection.h"
#include "sqlite_manager/statement.h"
#include "sqlite_manager/version.h"

namespace sqlite_manager_cli {

namespace {

using sqlite_manager::Connection;
using sqlite_manager::Statement;

struct Options {
    bool readonly = false;
    bool align = false;
    std::string database;
    std::string sql;
};

void PrintUsage(std::ostream& out, const ArgParser& parser) {
    out << "Usage: sqlite-manager [options] <database> <sql>\n"
           "  <database>   path to a database file, or \":memory:\"\n"
           "  <sql>        SQL to execute (quote it in the shell)\n"
           "Options:\n"
        << parser.HelpText();
}

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

CliApplication::CliApplication(std::ostream& out, std::ostream& err)
    : out_(out), err_(err) {}

int CliApplication::Run(int argc, char** argv) {
    bool help = false;
    bool version = false;
    Options opts;

    ArgParser parser;
    parser.Add({"--help", "-h"}, &help, "print this help and exit");
    parser.Add({"--version"}, &version,
               "print version information and exit");
    parser.Add({"--readonly"}, &opts.readonly,
               "open the database in read-only mode");
    parser.Add({"--align"}, &opts.align,
               "align SELECT output columns by width");

    if (!parser.Parse(argc, argv)) {
        err_ << parser.error() << "\n";
        PrintUsage(err_, parser);
        return 1;
    }

    if (help) {
        PrintUsage(out_, parser);
        return 0;
    }
    if (version) {
        out_ << "sqlite-manager "
             << sqlite_manager::kVersionMajor << '.'
             << sqlite_manager::kVersionMinor << '.'
             << sqlite_manager::kVersionPatch
             << " (SQLite " << sqlite_manager::SqliteVersion() << ")\n";
        return 0;
    }

    if (parser.positional().size() != 2) {
        err_ << "Expected <database> and <sql> arguments.\n";
        PrintUsage(err_, parser);
        return 1;
    }
    opts.database = parser.positional()[0];
    opts.sql = parser.positional()[1];

    Connection conn;
    const auto mode = opts.readonly
        ? Connection::OpenMode::kReadOnly
        : Connection::OpenMode::kReadWriteCreate;

    if (auto s = conn.Open(opts.database, mode); !s.ok()) {
        err_ << "Cannot open '" << opts.database
             << "': " << s.error().message << "\n";
        return 1;
    }

    // Single statements go through Statement so result rows can be
    // printed; batches fail Prepare and fall back to Execute.
    auto stmt = Statement::Prepare(conn, opts.sql);
    if (stmt.ok()) {
        if (stmt.value().ColumnCount() > 0) {
            return RunSelect(stmt.value(), opts.align, out_, err_);
        }
        auto step = stmt.value().Step();
        if (!step.ok()) {
            err_ << "Error: " << step.error().message << "\n";
            return 1;
        }
        out_ << "OK\n";
        return 0;
    }

    if (auto s = conn.Execute(opts.sql); !s.ok()) {
        err_ << "Error: " << s.error().message << "\n";
        return 1;
    }
    out_ << "OK\n";
    return 0;
}

}  // namespace sqlite_manager_cli