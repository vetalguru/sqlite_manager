#ifndef SQLITE_MANAGER_CLI_LINE_READER_H
#define SQLITE_MANAGER_CLI_LINE_READER_H

#include <iosfwd>
#include <optional>
#include <string>

namespace sqlite_manager_cli {

// Abstraction over "read one line of input with a prompt".
//
// Implementations:
//   StreamLineReader    plain std::getline over any istream; used by
//                       tests, piped input, and the --batch mode
//   IsoclineLineReader  interactive editing with history (isocline,
//                       cross-platform, handles multi-line paste)
class LineReader {
public:
    virtual ~LineReader() = default;

    // Shows `prompt` (implementation-defined) and reads one line.
    // Returns std::nullopt on end of input (EOF / Ctrl-D).
    virtual std::optional<std::string> ReadLine(const std::string& prompt) = 0;
};

// Reads lines from an istream; writes the prompt to an ostream
// unless prompts are disabled.
class StreamLineReader final : public LineReader {
public:
    StreamLineReader(std::istream& in, std::ostream& out, bool show_prompts);

    std::optional<std::string> ReadLine(const std::string& prompt) override;

private:
    std::istream& in_;
    std::ostream& out_;
    bool show_prompts_;
};

// Interactive reader with line editing and history persisted across
// sessions (in "$HOME/.sqlite_manager_history", or in-memory if no home
// directory is known). Reads from the real terminal (stdin); degrades
// to plain reads automatically when stdin is not a TTY (isocline).
class IsoclineLineReader final : public LineReader {
public:
    IsoclineLineReader();

    std::optional<std::string> ReadLine(const std::string& prompt) override;

private:
    std::string history_path_;   // empty when history is in-memory only
};

}  // namespace sqlite_manager_cli

#endif  // SQLITE_MANAGER_CLI_LINE_READER_H