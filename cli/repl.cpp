#include "repl.h"

#include <optional>
#include <ostream>

#include <sqlite3.h>

#include "line_reader.h"
#include "sql_executor.h"
#include "sqlite_manager/connection.h"

namespace sqlite_manager_cli {

namespace {

// True if the accumulated buffer forms one or more complete SQL
// statements. Delegates to sqlite3_complete(), which correctly ignores
// semicolons inside string literals, comments, and CREATE TRIGGER
// bodies - cases a naive last-character check would misjudge.
bool IsCompleteSql(const std::string& buffer) {
    return sqlite3_complete(buffer.c_str()) != 0;
}

// Trims leading and trailing whitespace.
std::string Trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

}  // namespace

Repl::Repl(sqlite_manager::Connection& conn,
           LineReader& reader, std::ostream& out, std::ostream& err)
    : conn_(conn), reader_(reader), out_(out), err_(err) {}

void Repl::PrintHelp() {
    out_ << "Enter SQL terminated by ';'. Dot commands:\n"
            "  .help     show this help\n"
            "  .tables   list tables in the database\n"
            "  .quit     exit the shell (also .exit or Ctrl-D)\n";
}

bool Repl::HandleDotCommand(const std::string& command) {
    if (command == ".quit" || command == ".exit") {
        return true;
    }
    if (command == ".help") {
        PrintHelp();
        return false;
    }
    if (command == ".tables") {
        ExecuteSql(conn_,
                   "SELECT name FROM sqlite_master "
                   "WHERE type = 'table' ORDER BY name;",
                   out_, err_);
        return false;
    }
    err_ << "Unknown command: " << command << " (try .help)\n";
    return false;
}

int Repl::Run() {
    std::string buffer;

    while (true) {
        const std::string prompt = buffer.empty() ? "sql> " : "...> ";
        std::optional<std::string> line = reader_.ReadLine(prompt);
        if (!line) break;   // EOF

        // Dot commands are recognized only at the start of a statement.
        if (buffer.empty()) {
            const std::string trimmed = Trim(*line);
            if (!trimmed.empty() && trimmed[0] == '.') {
                if (HandleDotCommand(trimmed)) {
                    return 0;
                }
                continue;
            }
        }

        buffer += *line;
        buffer += '\n';

        if (IsCompleteSql(buffer)) {
            ExecuteSql(conn_, buffer, out_, err_);
            buffer.clear();
        } else if (Trim(buffer).empty()) {
            buffer.clear();   // blank input, no continuation prompt
        }
    }

    // EOF with a non-empty buffer: execute what we have (mirrors the
    // official sqlite3 shell, which runs the pending input on exit).
    if (!Trim(buffer).empty()) {
        ExecuteSql(conn_, buffer, out_, err_);
    }
    return 0;
}

}  // namespace sqlite_manager_cli