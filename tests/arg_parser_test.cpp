#include "arg_parser.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace sqlite_manager_cli {
namespace {

// Builds a fake argv from string literals. argv[0] is the program name.
class ArgvBuilder {
public:
    explicit ArgvBuilder(std::vector<std::string> args)
        : storage_(std::move(args)) {
        pointers_.reserve(storage_.size() + 1);
        static char program_name[] = "test";
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

TEST(ArgParserTest, EmptyArgsParseToNothing) {
    ArgParser parser;
    ArgvBuilder args({});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_TRUE(parser.positional().empty());
}

TEST(ArgParserTest, SetsBoolFlag) {
    bool flag = false;
    ArgParser parser;
    parser.Add({"--flag"}, &flag, "");

    ArgvBuilder args({"--flag"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_TRUE(flag);
}

TEST(ArgParserTest, UntouchedFlagKeepsDefault) {
    bool flag = false;
    ArgParser parser;
    parser.Add({"--flag"}, &flag, "");

    ArgvBuilder args({"positional"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_FALSE(flag);
}

TEST(ArgParserTest, AliasesNameTheSameOption) {
    bool help = false;
    ArgParser parser;
    parser.Add({"--help", "-h"}, &help, "");

    ArgvBuilder args({"-h"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_TRUE(help);
}

TEST(ArgParserTest, FlagsWorkInAnyPosition) {
    bool a = false;
    bool b = false;
    ArgParser parser;
    parser.Add({"--a"}, &a, "");
    parser.Add({"--b"}, &b, "");

    ArgvBuilder args({"--a", "pos1", "--b", "pos2"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_TRUE(a);
    EXPECT_TRUE(b);
    ASSERT_EQ(parser.positional().size(), 2U);
    EXPECT_EQ(parser.positional()[0], "pos1");
    EXPECT_EQ(parser.positional()[1], "pos2");
}

TEST(ArgParserTest, StringOptionTakesNextToken) {
    std::string out;
    ArgParser parser;
    parser.Add({"--out"}, &out, "");

    ArgvBuilder args({"--out", "file.txt"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_EQ(out, "file.txt");
    EXPECT_TRUE(parser.positional().empty());
}

TEST(ArgParserTest, StringOptionTakesInlineValue) {
    std::string out;
    ArgParser parser;
    parser.Add({"--out"}, &out, "");

    ArgvBuilder args({"--out=file.txt"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_EQ(out, "file.txt");
}

TEST(ArgParserTest, InlineValueMayContainEquals) {
    std::string expr;
    ArgParser parser;
    parser.Add({"--set"}, &expr, "");

    ArgvBuilder args({"--set=a=b"});  // split on the FIRST '=' only
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_EQ(expr, "a=b");
}

TEST(ArgParserTest, LongOptionParsesInteger) {
    long limit = 0;
    ArgParser parser;
    parser.Add({"--limit"}, &limit, "");

    ArgvBuilder args({"--limit", "42"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_EQ(limit, 42);
}

TEST(ArgParserTest, LongOptionParsesNegative) {
    long value = 0;
    ArgParser parser;
    parser.Add({"--value"}, &value, "");

    ArgvBuilder args({"--value=-7"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_EQ(value, -7);
}

TEST(ArgParserTest, LongOptionRejectsGarbage) {
    long value = 0;
    ArgParser parser;
    parser.Add({"--value"}, &value, "");

    ArgvBuilder args({"--value", "12abc"});
    ASSERT_FALSE(parser.Parse(args.argc(), args.argv()));
    EXPECT_FALSE(parser.error().empty());
}

TEST(ArgParserTest, MissingValueIsAnError) {
    std::string out;
    ArgParser parser;
    parser.Add({"--out"}, &out, "");

    ArgvBuilder args({"--out"});
    ASSERT_FALSE(parser.Parse(args.argc(), args.argv()));
    EXPECT_NE(parser.error().find("--out"), std::string::npos);
}

TEST(ArgParserTest, FlagWithInlineValueIsAnError) {
    bool flag = false;
    ArgParser parser;
    parser.Add({"--flag"}, &flag, "");

    ArgvBuilder args({"--flag=true"});
    ASSERT_FALSE(parser.Parse(args.argc(), args.argv()));
}

TEST(ArgParserTest, UnknownOptionIsAnError) {
    ArgParser parser;
    ArgvBuilder args({"--nope"});
    ASSERT_FALSE(parser.Parse(args.argc(), args.argv()));
    EXPECT_NE(parser.error().find("--nope"), std::string::npos);
}

TEST(ArgParserTest, RepeatedOptionLastWins) {
    std::string out;
    ArgParser parser;
    parser.Add({"--out"}, &out, "");

    ArgvBuilder args({"--out", "first", "--out", "second"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_EQ(out, "second");
}

TEST(ArgParserTest, DoubleDashTerminatesOptions) {
    bool flag = false;
    ArgParser parser;
    parser.Add({"--flag"}, &flag, "");

    ArgvBuilder args({"--", "--flag", "-x"});
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    EXPECT_FALSE(flag);  // after "--" nothing is an option
    ASSERT_EQ(parser.positional().size(), 2U);
    EXPECT_EQ(parser.positional()[0], "--flag");
    EXPECT_EQ(parser.positional()[1], "-x");
}

TEST(ArgParserTest, SingleDashIsPositional) {
    ArgParser parser;
    ArgvBuilder args({"-"});  // conventionally means stdin
    ASSERT_TRUE(parser.Parse(args.argc(), args.argv()));
    ASSERT_EQ(parser.positional().size(), 1U);
    EXPECT_EQ(parser.positional()[0], "-");
}

TEST(ArgParserTest, ParseIsRepeatable) {
    bool flag = false;
    ArgParser parser;
    parser.Add({"--flag"}, &flag, "");

    ArgvBuilder first({"--flag", "one"});
    ASSERT_TRUE(parser.Parse(first.argc(), first.argv()));

    ArgvBuilder second({"two"});
    ASSERT_TRUE(parser.Parse(second.argc(), second.argv()));
    ASSERT_EQ(parser.positional().size(), 1U);  // cleared between runs
    EXPECT_EQ(parser.positional()[0], "two");
}

TEST(ArgParserTest, HelpTextListsAllOptions) {
    bool a = false;
    long n = 0;
    ArgParser parser;
    parser.Add({"--alpha", "-a"}, &a, "alpha flag");
    parser.Add({"--num"}, &n, "a number");

    const std::string help = parser.HelpText();
    EXPECT_NE(help.find("--alpha, -a"), std::string::npos);
    EXPECT_NE(help.find("alpha flag"), std::string::npos);
    EXPECT_NE(help.find("--num"), std::string::npos);
    EXPECT_NE(help.find("a number"), std::string::npos);
}

}  // namespace
}  // namespace sqlite_manager_cli