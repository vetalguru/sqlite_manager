#ifndef SQLITE_MANAGER_CLI_ARG_PARSER_H
#define SQLITE_MANAGER_CLI_ARG_PARSER_H

#include <string>
#include <variant>
#include <vector>

namespace sqlite_manager_cli {

// Minimal cross-platform command-line parser (pure C++17, no OS calls).
//
// Model: every option is a descriptor {aliases, destination, help}.
// The destination is a pointer to the caller's variable (push model);
// it must outlive Parse(). Supported destination types:
//   bool*         flag, no value:            --align
//   long*         integer value:             --limit=10  |  --limit 10
//   std::string*  string value:              --out=x.txt |  --out x.txt
//
// Rules:
//   - options may appear anywhere, in any order
//   - "--" terminates options: everything after it is positional
//   - repeated options: last one wins
//   - unknown "-something" is an error
//
// Usage:
//   ArgParser parser;
//   parser.Add({"--readonly", "-r"}, &opts.readonly, "open read-only");
//   parser.Add({"--limit"}, &opts.limit, "max rows to print");
//   if (!parser.Parse(argc, argv)) { std::cerr << parser.error(); }
class ArgParser final {
public:
    using Destination = std::variant<bool*, long*, std::string*>;

    // Registers an option under one or more aliases.
    // dest must outlive Parse(). help is used by HelpText().
    void Add(std::vector<std::string> aliases, Destination dest,
             std::string help);

    // Parses argv[1..argc). Returns false on the first error;
    // error() then contains a human-readable message.
    bool Parse(int argc, char** argv);

    const std::vector<std::string>& positional() const {
        return positional_;
    }
    const std::string& error() const { return error_; }

    // One line per option: aliases followed by its help string.
    std::string HelpText() const;

private:
    struct Option {
        std::vector<std::string> aliases;
        Destination dest;
        std::string help;
    };

    Option* Find(const std::string& name);

    std::vector<Option> options_;   // vector keeps --help in Add() order
    std::vector<std::string> positional_;
    std::string error_;
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_ARG_PARSER_H