#include <iostream>

#include "cli_application.h"

int main(int argc, char** argv) {
    sqlite_manager_cli::CliApplication app(std::cout, std::cerr);
    return app.Run(argc, argv);
}