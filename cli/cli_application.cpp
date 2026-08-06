#include "cli_application.h"

#include <iostream>
#include <istream>
#include <ostream>
#include <string>

#include "arg_parser.h"
#include "line_reader.h"
#include "repl.h"
#include "sql_executor.h"
#include "sqlite_manager/connection.h"
#include "sqlite_manager/version.h"

namespace sqlite_manager_cli {

namespace {

using sqlite_manager::Connection;

void PrintUsage(std::ostream& out, const ArgParser& parser) {
    out << "Usage: sqlite-manager [options] <database> [sql]\n"
           "  <database>   path to a database file, or \":memory:\"\n"
           "  [sql]        SQL to execute (quote it in the shell);\n"
           "               omit to enter the interactive shell\n"
           "Options:\n"
        << parser.HelpText();
}

}  // namespace

CliApplication::CliApplication(std::istream& in, std::ostream& out,
                               std::ostream& err)
    : in_(in), out_(out), err_(err) {}

int CliApplication::Run(int argc, char** argv) {
    bool help = false;
    bool version = false;
    bool readonly = false;
    bool batch = false;

    ArgParser parser;
    parser.Add({"--help", "-h"}, &help, "print this help and exit");
    parser.Add({"--version"}, &version,
               "print version information and exit");
    parser.Add({"--readonly"}, &readonly,
               "open the database in read-only mode");
    parser.Add({"--batch"}, &batch,
               "suppress interactive prompts");

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

    const auto& positional = parser.positional();
    if (positional.empty() || positional.size() > 2) {
        err_ << "Expected <database> and optional [sql] arguments.\n";
        PrintUsage(err_, parser);
        return 1;
    }

    Connection conn;
    const auto mode = readonly
        ? Connection::OpenMode::kReadOnly
        : Connection::OpenMode::kReadWriteCreate;

    if (auto s = conn.Open(positional[0], mode); !s.ok()) {
        err_ << "Cannot open '" << positional[0]
             << "': " << s.error().message << "\n";
        return 1;
    }

    if (positional.size() == 2) {
        return ExecuteSql(conn, positional[1], out_, err_);
    }

    // Real terminal session: use line editing with history. isocline
    // detects non-TTY stdin itself and degrades to plain reads.
    if (&in_ == &std::cin && !batch) {
        IsoclineLineReader reader;
        Repl repl(conn, reader, out_, err_);
        return repl.Run();
    }
    StreamLineReader reader(in_, out_, !batch);
    Repl repl(conn, reader, out_, err_);
    return repl.Run();
}

}  // namespace sqlite_manager_cli