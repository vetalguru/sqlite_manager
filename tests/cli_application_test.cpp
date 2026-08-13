#include "cli_application.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace sqlite_manager_cli {
namespace {

// Builds a fake argv. argv[0] is the program name.
class ArgvBuilder {
public:
    explicit ArgvBuilder(std::vector<std::string> args)
        : storage_(std::move(args)) {
        pointers_.reserve(storage_.size() + 1);
        static char program_name[] = "sqlite-manager";
        pointers_.push_back(program_name);
        for (std::string& arg : storage_) {
            pointers_.push_back(arg.data());
        }
    }

    int argc() const { return static_cast<int>(pointers_.size()); }
    char** argv() { return pointers_.data(); }

private:
    std::vector<std::string> storage_;
    std::vector<char*> pointers_;
};

// Runs the application with given args and stdin content,
// captures exit code and both output streams.
struct RunResult {
    int exit_code = -1;
    std::string out;
    std::string err;
};

RunResult RunApp(std::vector<std::string> args,
                 const std::string& input = "") {
    std::istringstream in(input);
    std::ostringstream out;
    std::ostringstream err;
    CliApplication app(in, out, err);

    ArgvBuilder argv(std::move(args));
    RunResult result;
    result.exit_code = app.Run(argv.argc(), argv.argv());
    result.out = out.str();
    result.err = err.str();
    return result;
}

// ---------- meta options ----------

TEST(CliApplicationTest, HelpPrintsUsageAndExitsZero) {
    const RunResult r = RunApp({"--help"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.out.find("Usage: sqlite-manager"), std::string::npos);
    EXPECT_NE(r.out.find("--readonly"), std::string::npos);
    EXPECT_TRUE(r.err.empty());
}

TEST(CliApplicationTest, VersionPrintsBothVersions) {
    const RunResult r = RunApp({"--version"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.out.find("sqlite-manager 0."), std::string::npos);
    EXPECT_NE(r.out.find("SQLite 3."), std::string::npos);
}

// ---------- argument errors ----------

TEST(CliApplicationTest, MissingArgumentsFails) {
    const RunResult r = RunApp({});
    EXPECT_EQ(r.exit_code, 1);
    EXPECT_NE(r.err.find("Expected <database> and optional [sql]"),
              std::string::npos);
    EXPECT_TRUE(r.out.empty());
}

TEST(CliApplicationTest, UnknownOptionFailsWithUsage) {
    const RunResult r = RunApp({"--nope", ":memory:", "SELECT 1"});
    EXPECT_EQ(r.exit_code, 1);
    EXPECT_NE(r.err.find("Unknown option: --nope"), std::string::npos);
    EXPECT_NE(r.err.find("Usage:"), std::string::npos);
}

// ---------- SQL execution (single-shot mode) ----------

TEST(CliApplicationTest, SelectPrintsFramedTableWithHeader) {
    const RunResult r = RunApp({":memory:", "SELECT 1+1 AS s, 'hi' AS t"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out,
              "+---+----+\n"
              "| s | t  |\n"
              "+---+----+\n"
              "| 2 | hi |\n"
              "+---+----+\n");
    EXPECT_TRUE(r.err.empty());
}

TEST(CliApplicationTest, NullPrintsAsNULL) {
    const RunResult r = RunApp({":memory:", "SELECT NULL AS v"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.out.find("| NULL |"), std::string::npos);
}

TEST(CliApplicationTest, DdlPrintsOk) {
    const RunResult r = RunApp({":memory:", "CREATE TABLE t (x)"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out, "OK\n");
}

TEST(CliApplicationTest, BatchPrintsOk) {
    const RunResult r = RunApp(
        {":memory:", "CREATE TABLE t (x); INSERT INTO t VALUES (1);"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out, "OK\n");
}

TEST(CliApplicationTest, InvalidSqlFails) {
    const RunResult r = RunApp({":memory:", "SELEKT 1"});
    EXPECT_EQ(r.exit_code, 1);
    EXPECT_NE(r.err.find("Error:"), std::string::npos);
    EXPECT_TRUE(r.out.empty());
}

TEST(CliApplicationTest, SqlCommentIsValidEmptyBatch) {
    const RunResult r = RunApp({":memory:", "--", "--just a comment"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out, "OK\n");
}

TEST(CliApplicationTest, DashSqlReachesSqliteNotParser) {
    const RunResult r = RunApp({":memory:", "--", "-broken"});
    EXPECT_EQ(r.exit_code, 1);
    EXPECT_EQ(r.err.find("Unknown option"), std::string::npos);
    EXPECT_NE(r.err.find("Error:"), std::string::npos);
}

// ---------- output format ----------

TEST(CliApplicationTest, ColumnsArePaddedToWidestValue) {
    const RunResult r = RunApp(
        {":memory:", "SELECT 1 AS a, 'x' AS b UNION ALL SELECT 22, 'y'"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out,
              "+----+---+\n"
              "| a  | b |\n"
              "+----+---+\n"
              "| 1  | x |\n"
              "| 22 | y |\n"
              "+----+---+\n");
}

TEST(CliApplicationTest, CsvFormatOutput) {
    const RunResult r = RunApp(
        {"--format", "csv", ":memory:",
         "SELECT 1 AS a, 'x' AS b UNION ALL SELECT 22, 'y'"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out, "a,b\n1,x\n22,y\n");
}

TEST(CliApplicationTest, CsvQuotesSpecialFieldsAndEmptiesNull) {
    const RunResult r = RunApp(
        {"--format", "csv", ":memory:", "SELECT 'a,b' AS c, NULL AS d"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out, "c,d\n\"a,b\",\n");
}

TEST(CliApplicationTest, JsonFormatOutput) {
    const RunResult r = RunApp(
        {"--format", "json", ":memory:",
         "SELECT 1 AS id, 'x' AS name UNION ALL SELECT 2, NULL"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out,
              "[\n"
              "  {\"id\": 1, \"name\": \"x\"},\n"
              "  {\"id\": 2, \"name\": null}\n"
              "]\n");
}

TEST(CliApplicationTest, JsonEmitsNumbersUnquoted) {
    const RunResult r = RunApp(
        {"--format", "json", ":memory:", "SELECT 42 AS i, 1.5 AS r"});
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out,
              "[\n"
              "  {\"i\": 42, \"r\": 1.5}\n"
              "]\n");
}

TEST(CliApplicationTest, UnknownFormatFails) {
    const RunResult r = RunApp({"--format", "xml", ":memory:", "SELECT 1"});
    EXPECT_EQ(r.exit_code, 1);
    EXPECT_NE(r.err.find("Unknown format"), std::string::npos);
    EXPECT_TRUE(r.out.empty());
}

// ---------- file database and readonly ----------

class CliApplicationFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = ::testing::TempDir() + "cli_app_test.sqlite";
        std::remove(path_.c_str());
    }
    void TearDown() override { std::remove(path_.c_str()); }

    std::string path_;
};

TEST_F(CliApplicationFileTest, DataPersistsBetweenRuns) {
    const RunResult create = RunApp(
        {path_, "CREATE TABLE t (x); INSERT INTO t VALUES (42);"});
    ASSERT_EQ(create.exit_code, 0);

    const RunResult read = RunApp({path_, "SELECT x FROM t"});
    EXPECT_EQ(read.exit_code, 0);
    EXPECT_NE(read.out.find("| 42 |"), std::string::npos);
}

TEST_F(CliApplicationFileTest, ReadonlyRejectsWrites) {
    ASSERT_EQ(RunApp({path_, "CREATE TABLE t (x)"}).exit_code, 0);

    const RunResult r = RunApp(
        {"--readonly", path_, "INSERT INTO t VALUES (1)"});
    EXPECT_EQ(r.exit_code, 1);
    EXPECT_NE(r.err.find("readonly"), std::string::npos);
}

TEST_F(CliApplicationFileTest, ReadonlyMissingFileFails) {
    const RunResult r = RunApp({"--readonly", path_, "SELECT 1"});
    EXPECT_EQ(r.exit_code, 1);
    EXPECT_NE(r.err.find("Cannot open"), std::string::npos);
}

// ---------- REPL mode (one positional argument) ----------

TEST(CliApplicationReplTest, ExecutesSqlAndQuits) {
    const RunResult r = RunApp({"--batch", ":memory:"},
                               "SELECT 1+1 AS v;\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out,
              "+---+\n"
              "| v |\n"
              "+---+\n"
              "| 2 |\n"
              "+---+\n");
    EXPECT_TRUE(r.err.empty());
}

TEST(CliApplicationReplTest, EofEndsTheShell) {
    const RunResult r = RunApp({"--batch", ":memory:"}, "SELECT 1;\n");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.out.find("| 1 |"), std::string::npos);
}

TEST(CliApplicationReplTest, MultilineSqlAccumulatesUntilSemicolon) {
    const RunResult r = RunApp({"--batch", ":memory:"},
                               "SELECT\n1+2\n;\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    // The unaliased expression names the column "1+2" (width 3),
    // so the value cell is padded: "| 3   |".
    EXPECT_NE(r.out.find("| 3  "), std::string::npos);
}

TEST(CliApplicationReplTest, SqlErrorDoesNotTerminateLoop) {
    const RunResult r = RunApp({"--batch", ":memory:"},
                               "SELEKT 1;\nSELECT 2;\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.err.find("Error:"), std::string::npos);
    EXPECT_NE(r.out.find("| 2 |"), std::string::npos);   // loop survived
}

TEST(CliApplicationReplTest, StatePersistsAcrossStatements) {
    const RunResult r = RunApp(
        {"--batch", ":memory:"},
        "CREATE TABLE t (x);\nINSERT INTO t VALUES (7);\n"
        "SELECT x FROM t;\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out,
              "OK\nOK\n"
              "+---+\n"
              "| x |\n"
              "+---+\n"
              "| 7 |\n"
              "+---+\n");
}

TEST(CliApplicationReplTest, TablesCommandListsTables) {
    const RunResult r = RunApp(
        {"--batch", ":memory:"},
        "CREATE TABLE bbb (x);\nCREATE TABLE aaa (x);\n.tables\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_EQ(r.out,
              "OK\nOK\n"
              "+------+\n"
              "| name |\n"
              "+------+\n"
              "| aaa  |\n"
              "| bbb  |\n"
              "+------+\n");
}

TEST(CliApplicationReplTest, HelpCommandPrintsCommands) {
    const RunResult r = RunApp({"--batch", ":memory:"}, ".help\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.out.find(".tables"), std::string::npos);
    EXPECT_NE(r.out.find(".quit"), std::string::npos);
}

TEST(CliApplicationReplTest, UnknownDotCommandReportsError) {
    const RunResult r = RunApp({"--batch", ":memory:"}, ".nope\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.err.find("Unknown command: .nope"), std::string::npos);
}

TEST(CliApplicationReplTest, PromptsAreShownWithoutBatch) {
    const RunResult r = RunApp({":memory:"}, "SELECT\n1;\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    // sql> then continuation then sql> after execution.
    EXPECT_NE(r.out.find("sql> "), std::string::npos);
    EXPECT_NE(r.out.find("...> "), std::string::npos);
}

TEST(CliApplicationReplTest, BatchSuppressesPrompts) {
    const RunResult r = RunApp({"--batch", ":memory:"},
                               "SELECT 1;\n.quit\n");
    EXPECT_EQ(r.out.find("sql>"), std::string::npos);
}

TEST(CliApplicationReplTest, PendingInputRunsOnEof) {
    // No trailing semicolon: the official shell executes pending
    // input on exit; so do we.
    const RunResult r = RunApp({"--batch", ":memory:"}, "SELECT 5");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.out.find("| 5 |"), std::string::npos);
}

TEST(CliApplicationReplTest, ReadonlyAppliesInRepl) {
    // :memory: opens fine in readonly (no file needed), but writing
    // into it must fail - verifies the mode reaches the REPL.
    const RunResult r = RunApp(
        {"--batch", "--readonly", ":memory:"},
        "CREATE TABLE t (x);\n.quit\n");
    EXPECT_EQ(r.exit_code, 0);
    EXPECT_NE(r.err.find("Error:"), std::string::npos);
}

}  // namespace
}  // namespace sqlite_manager_cli