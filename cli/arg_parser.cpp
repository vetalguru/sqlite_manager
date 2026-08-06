#include "arg_parser.h"

#include <charconv>

namespace sqlite_manager_cli {

namespace {

// Locale-independent integer conversion; requires the whole string
// to be consumed (rejects "12abc").
bool ParseLong(const std::string& text, long& out) {
    const char* first = text.data();
    const char* last = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, out);
    return ec == std::errc() && ptr == last;
}

}  // namespace

void ArgParser::Add(std::vector<std::string> aliases, Destination dest,
                    std::string help) {
    options_.push_back(Option{std::move(aliases), dest, std::move(help)});
}

ArgParser::Option* ArgParser::Find(const std::string& name) {
    for (Option& option : options_) {
        for (const std::string& alias : option.aliases) {
            if (alias == name) return &option;
        }
    }
    return nullptr;
}

bool ArgParser::Parse(int argc, char** argv) {
    positional_.clear();
    error_.clear();

    bool options_ended = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (options_ended) {
            positional_.push_back(arg);
            continue;
        }
        if (arg == "--") {
            options_ended = true;
            continue;
        }
        if (arg.empty() || arg[0] != '-' || arg == "-") {
            positional_.push_back(arg);
            continue;
        }

        // Split "--key=value" into name and inline value.
        std::string name = arg;
        std::string inline_value;
        bool has_inline_value = false;
        const std::size_t eq = arg.find('=');
        if (eq != std::string::npos) {
            name = arg.substr(0, eq);
            inline_value = arg.substr(eq + 1);
            has_inline_value = true;
        }

        Option* option = Find(name);
        if (option == nullptr) {
            error_ = "Unknown option: " + name;
            return false;
        }

        // Flags: no value allowed.
        if (auto* flag = std::get_if<bool*>(&option->dest)) {
            if (has_inline_value) {
                error_ = "Option " + name + " does not take a value";
                return false;
            }
            **flag = true;
            continue;
        }

        // Value options: take "=value" or the next argv token.
        std::string value;
        if (has_inline_value) {
            value = std::move(inline_value);
        } else if (i + 1 < argc) {
            value = argv[++i];
        } else {
            error_ = "Missing value for option " + name;
            return false;
        }

        if (auto* text = std::get_if<std::string*>(&option->dest)) {
            **text = std::move(value);
            continue;
        }
        if (auto* number = std::get_if<long*>(&option->dest)) {
            if (!ParseLong(value, **number)) {
                error_ = "Option " + name +
                         " expects an integer, got: " + value;
                return false;
            }
            continue;
        }
    }
    return true;
}

std::string ArgParser::HelpText() const {
    std::string text;
    for (const Option& option : options_) {
        std::string names;
        for (const std::string& alias : option.aliases) {
            if (!names.empty()) names += ", ";
            names += alias;
        }
        text += "  " + names;
        // Pad to a fixed column so help strings line up.
        constexpr std::size_t kHelpColumn = 20;
        if (names.size() + 2 < kHelpColumn) {
            text += std::string(kHelpColumn - names.size() - 2, ' ');
        } else {
            text += "  ";
        }
        text += option.help + "\n";
    }
    return text;
}

}  // namespace sqlite_manager_cli