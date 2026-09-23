#ifndef SQLITE_MANAGER_CLI_REPL_H
#define SQLITE_MANAGER_CLI_REPL_H

#include <iosfwd>
#include <string>

namespace sqlite_manager {
class Connection;
class ResultWriter;
}  // namespace sqlite_manager

namespace sqlite_manager_cli {

class LineReader;

// Interactive SQL shell over an open connection.
//
// Reads lines through a LineReader, accumulating them until the input
// forms a complete SQL statement (ends with ';'), then executes it.
// SQL errors are reported to `err` and do not terminate the loop.
// Dot commands (.help, .tables, .schema, .read, .quit/.exit) are
// recognized only at the start of a statement. EOF executes any pending
// input and exits.
class Repl final {
public:
    // The connection, reader, writer, and streams must outlive the object.
    // `errors_set_exit_code` is for non-interactive input (scripts, pipes):
    // any failed statement or dot command then makes Run() return 1.
    Repl(sqlite_manager::Connection& conn, LineReader& reader,
         const sqlite_manager::ResultWriter& writer, std::ostream& out,
         std::ostream& err, bool errors_set_exit_code);

    // Runs the loop until EOF or .quit. Returns the exit code: 1 if an
    // error occurred and errors set the exit code, 0 otherwise (an
    // interactive session ends normally regardless of errors inside).
    int Run();

private:
    // Executes SQL, recording a failure.
    void RunSql(const std::string& sql);
    // Marks the session as failed and returns the stream to report on.
    std::ostream& Fail();
    int ExitCode() const;

    // Returns true if the REPL should exit.
    bool HandleDotCommand(const std::string& command);
    void PrintHelp();
    // Prints the CREATE statements for all objects, or just `table`.
    void PrintSchema(const std::string& table);
    // Reads `path` and executes the SQL statements it contains.
    void ReadFile(const std::string& path);
    // Executes a string of SQL, statement by statement.
    void ExecuteScript(const std::string& script);

    sqlite_manager::Connection& conn_;
    LineReader& reader_;
    const sqlite_manager::ResultWriter& writer_;
    std::ostream& out_;
    std::ostream& err_;
    bool errors_set_exit_code_;
    bool had_error_ = false;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_REPL_H