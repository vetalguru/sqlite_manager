#ifndef SQLITE_MANAGER_CLI_CLI_APPLICATION_H
#define SQLITE_MANAGER_CLI_CLI_APPLICATION_H

#include <iosfwd>
#include <string>

namespace sqlite_manager_cli {

class CliApplication final {
public:
    CliApplication(std::ostream& out, std::ostream& err);

    int Run(int argc, char** argv);

private:
    std::ostream& out_;
    std::ostream& err_;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_CLI_APPLICATION_H