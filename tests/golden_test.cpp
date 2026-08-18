// Data-driven functional tests for the CLI, one "golden" file per case.
//
// Each *.tst file under tests/golden/ describes a full run of the product
// (a seeded database, a command line and/or REPL input) together with the
// exact output it must produce. The reference answer is what the CLI
// prints - the layer we own - not a raw SQL result set, so these exercise
// argument parsing, execution and the table/CSV/JSON writers end to end.
//
// File format (see tests/golden/README.md for the full description):
//
//   === name            one-line human name (optional; default: file path)
//   === db              memory | file | file-ro | none   (default: memory)
//   === setup           SQL seeded before the run (file / file-ro only)
//   === file            aux file contents; "{FILE}" resolves to its path
//   === args            extra CLI option tokens, one per line
//   === sql             positional SQL argument (single-shot mode)
//   === stdin           REPL input fed on standard input
//   === exit            expected exit code (default: 0)
//   === out             expected stdout, exact
//   === err             expected stderr, exact
//   === err-contains    substring stderr must contain
//
// A body is the lines below its header up to the next header, each joined
// with a trailing newline - matching the CLI, whose output is always
// newline-terminated. If neither err nor err-contains is given, stderr
// must be empty.

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "cli_application.h"
#include "sqlite_manager/connection.h"

namespace sqlite_manager_cli {
namespace {

namespace fs = std::filesystem;

// The golden corpus root, injected by CMake as a quoted string literal.
#ifndef SQLITE_MANAGER_GOLDEN_DIR
#define SQLITE_MANAGER_GOLDEN_DIR "golden"
#endif

// One parsed golden file: section key -> its body lines, in file order.
struct GoldenCase {
    std::map<std::string, std::vector<std::string>> sections;

    bool has(const std::string& key) const {
        return sections.find(key) != sections.end();
    }
    const std::vector<std::string>& body(const std::string& key) const {
        static const std::vector<std::string> kEmpty;
        const auto it = sections.find(key);
        return it == sections.end() ? kEmpty : it->second;
    }
    // First body line of a single-valued section, or a fallback.
    std::string line(const std::string& key,
                     const std::string& fallback) const {
        const auto& b = body(key);
        return b.empty() ? fallback : b.front();
    }
    // Body as exact expected text: every line newline-terminated.
    std::string text(const std::string& key) const {
        std::string out;
        for (const std::string& l : body(key)) {
            out += l;
            out += '\n';
        }
        return out;
    }
};

GoldenCase ParseGolden(const std::string& path) {
    GoldenCase c;
    std::ifstream in(path);
    std::string raw;
    std::string current;  // empty until the first "=== " header
    while (std::getline(in, raw)) {
        // Normalize a trailing CR so files edited on Windows still match.
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();

        if (raw.rfind("=== ", 0) == 0) {
            current = raw.substr(4);
            // Trim trailing spaces from the key.
            while (!current.empty() && current.back() == ' ') {
                current.pop_back();
            }
            c.sections[current];  // create an (empty) section
            continue;
        }
        if (current.empty()) continue;  // pre-section comment / blank
        c.sections[current].push_back(raw);
    }
    return c;
}

// Builds a throwaway argv (argv[0] is the program name).
class ArgvBuilder {
public:
    explicit ArgvBuilder(std::vector<std::string> args)
        : storage_(std::move(args)) {
        pointers_.reserve(storage_.size() + 1);
        static char program_name[] = "sqlite-manager";
        pointers_.push_back(program_name);
        for (std::string& arg : storage_) pointers_.push_back(arg.data());
    }
    int argc() const { return static_cast<int>(pointers_.size()); }
    char** argv() { return pointers_.data(); }

private:
    std::vector<std::string> storage_;
    std::vector<char*> pointers_;
};

// A filesystem-safe stem for a per-case temp database.
std::string SafeStem(const std::string& path) {
    std::string s = fs::path(path).filename().string();
    for (char& ch : s) {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0) ch = '_';
    }
    return s;
}

// Replaces every occurrence of `from` in `s` with `to`.
std::string ReplaceAll(std::string s, const std::string& from,
                       const std::string& to) {
    if (from.empty()) return s;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

void RunGolden(const std::string& path) {
    const GoldenCase c = ParseGolden(path);

    const std::string db = c.line("db", "memory");
    const bool is_file = (db == "file" || db == "file-ro");

    ASSERT_TRUE(db == "memory" || db == "none" || is_file)
        << "unknown db kind: " << db;
    ASSERT_TRUE(is_file || !c.has("setup"))
        << "setup requires db: file or file-ro";

    // Resolve the database target and seed it if requested.
    std::string db_path;
    if (is_file) {
        db_path = ::testing::TempDir() + "golden_" + SafeStem(path) + ".sqlite";
        std::remove(db_path.c_str());
        if (c.has("setup")) {
            sqlite_manager::Connection seed;
            ASSERT_TRUE(seed.Open(db_path).ok()) << "cannot create " << db_path;
            std::string sql;
            for (const std::string& l : c.body("setup")) {
                sql += l;
                sql += '\n';
            }
            const sqlite_manager::Status s = seed.Execute(sql);
            ASSERT_TRUE(s.ok()) << "setup failed: " << s.error().message;
        }
    } else if (db == "memory") {
        db_path = ":memory:";
    }

    // Optional auxiliary file; "{FILE}" in args/sql/stdin resolves to it.
    std::string aux_path;
    if (c.has("file")) {
        aux_path =
            ::testing::TempDir() + "golden_aux_" + SafeStem(path) + ".txt";
        std::ofstream aux(aux_path);
        for (const std::string& l : c.body("file")) aux << l << '\n';
    }
    const auto resolve = [&](const std::string& s) {
        return aux_path.empty() ? s : ReplaceAll(s, "{FILE}", aux_path);
    };

    // Assemble the command line: [options] [db] [sql].
    std::vector<std::string> args;
    if (db == "file-ro") args.emplace_back("--readonly");
    for (const std::string& tok : c.body("args")) {
        if (!tok.empty()) args.push_back(resolve(tok));
    }
    if (db != "none") args.push_back(db_path);
    if (c.has("sql")) {
        // A multi-line SQL body joins without a trailing newline.
        std::string sql;
        const auto& lines = c.body("sql");
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) sql += '\n';
            sql += lines[i];
        }
        args.push_back(resolve(sql));
    }

    // Standard input for REPL cases.
    std::string input;
    for (const std::string& l : c.body("stdin")) {
        input += resolve(l);
        input += '\n';
    }

    std::istringstream cin_stream(input);
    std::ostringstream out;
    std::ostringstream err;
    CliApplication app(cin_stream, out, err);
    ArgvBuilder argv(std::move(args));
    const int exit_code = app.Run(argv.argc(), argv.argv());

    if (is_file) std::remove(db_path.c_str());
    if (!aux_path.empty()) std::remove(aux_path.c_str());

    const int want_exit = std::stoi(c.line("exit", "0"));
    EXPECT_EQ(exit_code, want_exit) << "stderr was:\n" << err.str();

    if (c.has("out")) EXPECT_EQ(out.str(), c.text("out"));

    if (c.has("err")) {
        EXPECT_EQ(err.str(), c.text("err"));
    } else if (c.has("err-contains")) {
        EXPECT_NE(err.str().find(c.line("err-contains", "")), std::string::npos)
            << "stderr was:\n"
            << err.str();
    } else {
        EXPECT_EQ(err.str(), "") << "unexpected stderr";
    }
}

// Discover every *.tst under the corpus root at static-init time so each
// file becomes its own parameterized test case.
std::vector<std::string> DiscoverGoldenFiles() {
    std::vector<std::string> files;
    const fs::path root = SQLITE_MANAGER_GOLDEN_DIR;
    std::error_code ec;
    if (fs::exists(root, ec)) {
        for (const auto& entry : fs::recursive_directory_iterator(root, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".tst") {
                files.emplace_back(entry.path().string());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

// A gtest-legal, unique name from the path relative to the corpus root.
std::string CaseName(const ::testing::TestParamInfo<std::string>& info) {
    fs::path rel = fs::relative(info.param, SQLITE_MANAGER_GOLDEN_DIR);
    rel.replace_extension();
    std::string name = rel.string();
    for (char& ch : name) {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0) ch = '_';
    }
    return name;
}

class GoldenTest : public ::testing::TestWithParam<std::string> {};

TEST_P(GoldenTest, MatchesReferenceOutput) {
    SCOPED_TRACE(GetParam());
    RunGolden(GetParam());
}

INSTANTIATE_TEST_SUITE_P(Corpus, GoldenTest,
                         ::testing::ValuesIn(DiscoverGoldenFiles()), CaseName);

}  // namespace
}  // namespace sqlite_manager_cli
