#include "sqlite_manager/sql_util.h"

#include <gtest/gtest.h>

#include <string>

namespace sqlite_manager {
namespace {

TEST(SqlUtilTest, TerminatedStatementIsComplete) {
    EXPECT_TRUE(IsCompleteStatement("SELECT 1;"));
    EXPECT_TRUE(IsCompleteStatement("SELECT 1;\n"));
    EXPECT_TRUE(IsCompleteStatement("CREATE TABLE t (x);"));
}

TEST(SqlUtilTest, UnterminatedStatementIsIncomplete) {
    EXPECT_FALSE(IsCompleteStatement("SELECT 1"));
    EXPECT_FALSE(IsCompleteStatement("CREATE TABLE t (x)"));
}

TEST(SqlUtilTest, BlankOrCommentOnlyIsIncomplete) {
    EXPECT_FALSE(IsCompleteStatement(""));
    EXPECT_FALSE(IsCompleteStatement("   \n\t "));
    EXPECT_FALSE(IsCompleteStatement("-- just a comment\n"));
}

TEST(SqlUtilTest, SemicolonInsideStringDoesNotEndTheStatement) {
    EXPECT_FALSE(IsCompleteStatement("SELECT 'a;b'"));
    EXPECT_TRUE(IsCompleteStatement("SELECT 'a;b';"));
}

TEST(SqlUtilTest, TriggerBodyIsCompleteOnlyAfterEnd) {
    // Internal semicolons inside BEGIN ... END must not end the statement.
    EXPECT_FALSE(IsCompleteStatement(
        "CREATE TRIGGER t AFTER UPDATE ON x BEGIN UPDATE x SET y = 1;"));
    EXPECT_TRUE(IsCompleteStatement(
        "CREATE TRIGGER t AFTER UPDATE ON x BEGIN UPDATE x SET y = 1; END;"));
}

TEST(SqlUtilTest, QuoteIdentifierWrapsAndEscapes) {
    EXPECT_EQ(QuoteIdentifier("users"), "\"users\"");
    EXPECT_EQ(QuoteIdentifier("weird name"), "\"weird name\"");
    EXPECT_EQ(QuoteIdentifier("a\"b"), "\"a\"\"b\"");
    EXPECT_EQ(QuoteIdentifier(""), "\"\"");
}

}  // namespace
}  // namespace sqlite_manager
