#include "line_reader.h"

#include <isocline.h>

#include <cstdlib>
#include <istream>
#include <ostream>

namespace sqlite_manager_cli {

StreamLineReader::StreamLineReader(std::istream& in, std::ostream& out,
                                   bool show_prompts)
    : in_(in), out_(out), show_prompts_(show_prompts) {}

std::optional<std::string> StreamLineReader::ReadLine(
    const std::string& prompt) {
    if (show_prompts_) out_ << prompt;
    std::string line;
    if (!std::getline(in_, line)) {
        if (show_prompts_) out_ << "\n";
        return std::nullopt;
    }
    return line;
}

IsoclineLineReader::IsoclineLineReader() {
    // Persist history across sessions under the user's home directory.
    const char* home = std::getenv("HOME");
    if (home == nullptr) home = std::getenv("USERPROFILE");  // Windows
    if (home != nullptr) {
        history_path_ = std::string(home) + "/.sqlite_manager_history";
    }
    ic_set_history(history_path_.empty() ? nullptr : history_path_.c_str(),
                   1000);           // up to 1000 entries
    ic_enable_multiline(false);     // our Repl handles multi-line SQL
    ic_set_prompt_marker("", "");   // no extra marker, prompt as given
}

std::optional<std::string> IsoclineLineReader::ReadLine(
    const std::string& prompt) {
    char* raw = ic_readline(prompt.c_str());
    if (raw == nullptr) {
        return std::nullopt;  // EOF / Ctrl-D
    }
    std::string line = raw;
    // isocline allocates with malloc; we own the buffer.
    std::free(raw);
    if (!line.empty()) {
        ic_history_add(line.c_str());
    }
    return line;
}

}  // namespace sqlite_manager_cli