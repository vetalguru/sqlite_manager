#include "repl.h"

#include <fstream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>

#include "line_reader.h"
#include "sql_executor.h"
#include "sqlite_manager/connection.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_cli {

namespace {

using sqlite_manager::Statement;

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

Repl::Repl(sqlite_manager::Connection& conn, LineReader& reader,
           const sqlite_manager::ResultWriter& writer, std::ostream& out,
           std::ostream& err)
    : conn_(conn), reader_(reader), writer_(writer), out_(out), err_(err) {}

void Repl::PrintHelp() {
    out_ << "Enter SQL terminated by ';'. Dot commands:\n"
            "  .help            show this help\n"
            "  .tables          list tables in the database\n"
            "  .schema [TABLE]  show CREATE statements (all, or one table)\n"
            "  .read FILE       execute SQL statements from FILE\n"
            "  .quit            exit the shell (also .exit or Ctrl-D)\n";
}

bool Repl::HandleDotCommand(const std::string& command) {
    // Split into the command word and its optional argument.
    const auto sep = command.find_first_of(" \t");
    const std::string cmd = command.substr(0, sep);
    const std::string arg =
        (sep == std::string::npos) ? "" : Trim(command.substr(sep + 1));

    if (cmd == ".quit" || cmd == ".exit") {
        return true;
    }
    if (cmd == ".help") {
        PrintHelp();
        return false;
    }
    if (cmd == ".tables") {
        ExecuteSql(conn_,
                   "SELECT name FROM sqlite_master "
                   "WHERE type = 'table' ORDER BY name;",
                   writer_, out_, err_);
        return false;
    }
    if (cmd == ".schema") {
        PrintSchema(arg);
        return false;
    }
    if (cmd == ".read") {
        if (arg.empty()) {
            err_ << ".read requires a file path\n";
        } else {
            ReadFile(arg);
        }
        return false;
    }
    err_ << "Unknown command: " << command << " (try .help)\n";
    return false;
}

void Repl::PrintSchema(const std::string& table) {
    std::string sql = "SELECT sql FROM sqlite_master WHERE sql IS NOT NULL";
    if (!table.empty()) {
        sql += " AND name = :name";
    }
    sql += " ORDER BY rowid;";

    auto stmt = Statement::Prepare(conn_, sql);
    if (!stmt.ok()) {
        err_ << "Error: " << stmt.error().message << "\n";
        return;
    }
    if (!table.empty()) {
        if (const auto bound = stmt.value().BindText(":name", table);
            !bound.ok()) {
            err_ << "Error: " << bound.error().message << "\n";
            return;
        }
    }

    bool found = false;
    while (true) {
        auto step = stmt.value().Step();
        if (!step.ok()) {
            err_ << "Error: " << step.error().message << "\n";
            return;
        }
        if (step.value() == Statement::StepResult::kDone) break;
        // sqlite_master stores CREATE statements without a terminator.
        out_ << stmt.value().ColumnText(0) << ";\n";
        found = true;
    }
    if (!table.empty() && !found) {
        err_ << "No such table: " << table << "\n";
    }
}

void Repl::ReadFile(const std::string& path) {
    const std::ifstream file(path);
    if (!file) {
        err_ << "Cannot open: " << path << "\n";
        return;
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    ExecuteScript(contents.str());
}

void Repl::ExecuteScript(const std::string& script) {
    std::istringstream in(script);
    std::string buffer;
    std::string line;
    while (std::getline(in, line)) {
        buffer += line;
        buffer += '\n';
        if (IsCompleteSql(buffer)) {
            ExecuteSql(conn_, buffer, writer_, out_, err_);
            buffer.clear();
        }
    }
    // Run any trailing statement missing its final semicolon.
    if (!Trim(buffer).empty()) {
        ExecuteSql(conn_, buffer, writer_, out_, err_);
    }
}

int Repl::Run() {
    std::string buffer;

    while (true) {
        const std::string prompt = buffer.empty() ? "sql> " : "...> ";
        std::optional<std::string> line = reader_.ReadLine(prompt);
        if (!line) break;  // EOF

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
            ExecuteSql(conn_, buffer, writer_, out_, err_);
            buffer.clear();
        } else if (Trim(buffer).empty()) {
            buffer.clear();  // blank input, no continuation prompt
        }
    }

    // EOF with a non-empty buffer: execute what we have (mirrors the
    // official sqlite3 shell, which runs the pending input on exit).
    if (!Trim(buffer).empty()) {
        ExecuteSql(conn_, buffer, writer_, out_, err_);
    }
    return 0;
}

}  // namespace sqlite_manager_cli