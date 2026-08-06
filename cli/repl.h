#ifndef SQLITE_MANAGER_CLI_REPL_H
#define SQLITE_MANAGER_CLI_REPL_H

#include <iosfwd>
#include <string>

namespace sqlite_manager {
class Connection;
}

namespace sqlite_manager_cli {

// Interactive SQL shell over an open connection.
//
// Reads lines from `in`, accumulating them until the input forms a
// complete SQL statement (ends with ';'), then executes it. SQL
// errors are reported to `err` and do not terminate the loop.
// Dot commands (.help, .tables, .quit/.exit) are recognized only at
// the start of a statement. EOF executes any pending input and exits.
class Repl final {
public:
    struct Config {
        bool align = false;   // align SELECT output columns
        bool batch = false;   // suppress "sql> " / "...> " prompts
    };

    // The connection and streams must outlive the object.
    Repl(sqlite_manager::Connection& conn, Config config,
         std::istream& in, std::ostream& out, std::ostream& err);

    // Runs the loop until EOF or .quit. Returns the exit code
    // (0: session ended normally, regardless of SQL errors inside).
    int Run();

private:
    // Returns true if the REPL should exit.
    bool HandleDotCommand(const std::string& command);
    void PrintHelp();

    sqlite_manager::Connection& conn_;
    Config config_;
    std::istream& in_;
    std::ostream& out_;
    std::ostream& err_;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_REPL_H