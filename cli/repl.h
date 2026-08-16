#ifndef SQLITE_MANAGER_CLI_REPL_H
#define SQLITE_MANAGER_CLI_REPL_H

#include <iosfwd>
#include <string>

namespace sqlite_manager {
class Connection;
}

namespace sqlite_manager_cli {

class LineReader;
class ResultView;

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
    // The connection, reader, view, and streams must outlive the object.
    Repl(sqlite_manager::Connection& conn, LineReader& reader,
         const ResultView& view, std::ostream& out, std::ostream& err);

    // Runs the loop until EOF or .quit. Returns the exit code
    // (0: session ended normally, regardless of SQL errors inside).
    int Run();

private:
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
    const ResultView& view_;
    std::ostream& out_;
    std::ostream& err_;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_REPL_H