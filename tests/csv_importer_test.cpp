#include "gui/core/csv_importer.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_gui {
namespace {

using sqlite_manager::Connection;

std::string Scalar(Connection& conn, const std::string& sql) {
    auto stmt = sqlite_manager::Statement::Prepare(conn, sql);
    EXPECT_TRUE(stmt.ok());
    auto step = stmt.value().Step();
    EXPECT_TRUE(step.ok());
    return stmt.value().ColumnText(0);
}

// ---------- ParseCsv ----------

TEST(ParseCsvTest, SplitsSimpleRows) {
    auto grid = ParseCsv("a,b,c\n1,2,3\n");
    ASSERT_EQ(grid.size(), 2U);
    EXPECT_EQ(grid[0], (std::vector<std::string>{"a", "b", "c"}));
    EXPECT_EQ(grid[1], (std::vector<std::string>{"1", "2", "3"}));
}

TEST(ParseCsvTest, HandlesQuotedFields) {
    auto grid = ParseCsv("\"x,y\",\"he said \"\"hi\"\"\",\"line1\nline2\"\n");
    ASSERT_EQ(grid.size(), 1U);
    ASSERT_EQ(grid[0].size(), 3U);
    EXPECT_EQ(grid[0][0], "x,y");
    EXPECT_EQ(grid[0][1], "he said \"hi\"");
    EXPECT_EQ(grid[0][2], "line1\nline2");
}

TEST(ParseCsvTest, HandlesCrlfAndTrailingField) {
    auto grid = ParseCsv("a,b\r\n1,\r\n");
    ASSERT_EQ(grid.size(), 2U);
    EXPECT_EQ(grid[1], (std::vector<std::string>{"1", ""}));
}

TEST(ParseCsvTest, LastLineWithoutNewline) {
    auto grid = ParseCsv("a,b\n1,2");
    ASSERT_EQ(grid.size(), 2U);
    EXPECT_EQ(grid[1], (std::vector<std::string>{"1", "2"}));
}

TEST(ParseCsvTest, EmptyInputIsNoRows) { EXPECT_TRUE(ParseCsv("").empty()); }

// ---------- ImportCsv ----------

Connection MakeDb() {
    Connection conn;
    EXPECT_TRUE(conn.Open(":memory:").ok());
    EXPECT_TRUE(
        conn.Execute("CREATE TABLE t (id INTEGER, name TEXT, qty INTEGER);")
            .ok());
    return conn;
}

TEST(ImportCsvTest, ImportsRowsWithHeader) {
    auto conn = MakeDb();
    auto count = ImportCsv(conn, "t", "id,name,qty\n1,a,10\n2,b,20\n",
                           /*has_header=*/true);
    ASSERT_TRUE(count.ok());
    EXPECT_EQ(count.value(), 2);
    EXPECT_EQ(Scalar(conn, "SELECT count(*) FROM t;"), "2");
    EXPECT_EQ(Scalar(conn, "SELECT name FROM t WHERE id = 2;"), "b");
    // Integer affinity applies to imported text.
    EXPECT_EQ(Scalar(conn, "SELECT typeof(qty) FROM t WHERE id = 1;"),
              "integer");
}

TEST(ImportCsvTest, EmptyFieldBecomesNull) {
    auto conn = MakeDb();
    auto count = ImportCsv(conn, "t", "id,name,qty\n1,,5\n", true);
    ASSERT_TRUE(count.ok());
    EXPECT_EQ(Scalar(conn, "SELECT typeof(name) FROM t WHERE id = 1;"), "null");
}

TEST(ImportCsvTest, PositionalWithoutHeader) {
    auto conn = MakeDb();
    auto count = ImportCsv(conn, "t", "1,a,10\n2,b,20\n", /*has_header=*/false);
    ASSERT_TRUE(count.ok());
    EXPECT_EQ(count.value(), 2);
    EXPECT_EQ(Scalar(conn, "SELECT count(*) FROM t;"), "2");
}

TEST(ImportCsvTest, RollsBackOnError) {
    auto conn = MakeDb();
    // Second row targets a non-existent column via a bad header.
    auto count = ImportCsv(conn, "t", "id,nope\n1,x\n", true);
    EXPECT_FALSE(count.ok());
    EXPECT_EQ(Scalar(conn, "SELECT count(*) FROM t;"), "0");
}

}  // namespace
}  // namespace sqlite_manager_gui
