#ifndef SQLITE_MANAGER_CLI_CLI_APPLICATION_H
#define SQLITE_MANAGER_CLI_CLI_APPLICATION_H

#include <iosfwd>

namespace sqlite_manager_cli {

// The whole CLI in a testable object:
// main() only constructs it and calls Run().
//
// All input and output are injected: argv comes in as arguments,
// text flows through the provided streams. This makes the full
// application logic testable with in-memory streams, no processes.
//
// Modes (selected by the number of positional arguments):
//   <database> <sql>   execute one SQL string and exit
//   <database>         interactive REPL reading SQL from in
class CliApplication final {
public:
    // Streams must outlive the object.
    // Production: std::cin/std::cout/std::cerr.
    // Tests: std::istringstream / std::ostringstream instances.
    CliApplication(std::istream& in, std::ostream& out, std::ostream& err);

    // Runs the application. Returns the process exit code.
    int Run(int argc, char** argv);

private:
    std::istream& in_;
    std::ostream& out_;
    std::ostream& err_;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_CLI_APPLICATION_H