#include "repl.h"

#include <istream>
#include <ostream>

#include "sql_executor.h"
#include "sqlite_manager/connection.h"

namespace sqlite_manager_cli {

namespace {

// True if the accumulated buffer forms a complete SQL input:
// its last non-whitespace character is a semicolon.
bool IsCompleteSql(const std::string& buffer) {
    const auto last = buffer.find_last_not_of(" \t\r\n");
    return last != std::string::npos && buffer[last] == ';';
}

// Trims leading and trailing whitespace.
std::string Trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

}  // namespace

Repl::Repl(sqlite_manager::Connection& conn, Config config,
           std::istream& in, std::ostream& out, std::ostream& err)
    : conn_(conn), config_(config), in_(in), out_(out), err_(err) {}

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
                   config_.align, out_, err_);
        return false;
    }
    err_ << "Unknown command: " << command << " (try .help)\n";
    return false;
}

int Repl::Run() {
    std::string buffer;
    std::string line;

    if (!config_.batch) out_ << "sql> ";
    while (std::getline(in_, line)) {
        // Dot commands are recognized only at the start of a statement.
        if (buffer.empty()) {
            const std::string trimmed = Trim(line);
            if (!trimmed.empty() && trimmed[0] == '.') {
                if (HandleDotCommand(trimmed)) {
                    return 0;
                }
                if (!config_.batch) out_ << "sql> ";
                continue;
            }
        }

        buffer += line;
        buffer += '\n';

        if (IsCompleteSql(buffer)) {
            ExecuteSql(conn_, buffer, config_.align, out_, err_);
            buffer.clear();
        } else if (Trim(buffer).empty()) {
            buffer.clear();   // blank input, no continuation prompt
        }

        if (!config_.batch) out_ << (buffer.empty() ? "sql> " : "...> ");
    }

    // EOF with a non-empty buffer: execute what we have (mirrors the
    // official sqlite3 shell, which runs the pending input on exit).
    if (!Trim(buffer).empty()) {
        ExecuteSql(conn_, buffer, config_.align, out_, err_);
    }
    if (!config_.batch) out_ << "\n";
    return 0;
}

}  // namespace sqlite_manager_cli