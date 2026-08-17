#include "gui/core/row_editor.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_gui {
namespace {

using sqlite_manager::Cell;
using sqlite_manager::Connection;
using sqlite_manager::ValueType;

Cell Text(std::string text) { return {ValueType::kText, std::move(text)}; }
Cell Null() { return {ValueType::kNull, {}}; }

Connection MakeDb() {
    Connection conn;
    EXPECT_TRUE(conn.Open(":memory:").ok());
    EXPECT_TRUE(
        conn.Execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, qty "
                     "INTEGER);"
                     "INSERT INTO t (name, qty) VALUES ('a', 1), ('b', 2);")
            .ok());
    return conn;
}

// First column of the first row of a query, as text.
std::string Scalar(Connection& conn, const std::string& sql) {
    auto stmt = sqlite_manager::Statement::Prepare(conn, sql);
    EXPECT_TRUE(stmt.ok());
    auto step = stmt.value().Step();
    EXPECT_TRUE(step.ok());
    return stmt.value().ColumnText(0);
}

TEST(RowEditorTest, UpdateCellChangesTextValue) {
    auto conn = MakeDb();
    ASSERT_TRUE(UpdateCell(conn, "t", 1, "name", Text("z")).ok());
    EXPECT_EQ(Scalar(conn, "SELECT name FROM t WHERE id = 1;"), "z");
}

TEST(RowEditorTest, UpdateCellAppliesColumnAffinity) {
    auto conn = MakeDb();
    // Text "99" into an INTEGER column is stored as an integer.
    ASSERT_TRUE(UpdateCell(conn, "t", 1, "qty", Text("99")).ok());
    EXPECT_EQ(Scalar(conn, "SELECT typeof(qty) FROM t WHERE id = 1;"),
              "integer");
    EXPECT_EQ(Scalar(conn, "SELECT qty FROM t WHERE id = 1;"), "99");
}

TEST(RowEditorTest, UpdateCellCanSetNull) {
    auto conn = MakeDb();
    ASSERT_TRUE(UpdateCell(conn, "t", 1, "name", Null()).ok());
    EXPECT_EQ(Scalar(conn, "SELECT typeof(name) FROM t WHERE id = 1;"), "null");
}

TEST(RowEditorTest, DeleteRowRemovesIt) {
    auto conn = MakeDb();
    ASSERT_TRUE(DeleteRow(conn, "t", 1).ok());
    EXPECT_EQ(Scalar(conn, "SELECT count(*) FROM t;"), "1");
}

TEST(RowEditorTest, InsertRowReturnsNewRowid) {
    auto conn = MakeDb();
    std::vector<std::pair<std::string, Cell>> values = {{"name", Text("c")},
                                                        {"qty", Text("3")}};
    auto rowid = InsertRow(conn, "t", values);
    ASSERT_TRUE(rowid.ok());
    EXPECT_GT(rowid.value(), 0);
    EXPECT_EQ(Scalar(conn, "SELECT name FROM t WHERE id = " +
                               std::to_string(rowid.value()) + ";"),
              "c");
}

TEST(RowEditorTest, InsertRowWithNoValuesUsesDefaults) {
    auto conn = MakeDb();
    auto rowid = InsertRow(conn, "t", {});
    ASSERT_TRUE(rowid.ok());
    EXPECT_EQ(Scalar(conn, "SELECT count(*) FROM t;"), "3");
}

TEST(RowEditorTest, QuotesIdentifiersSafely) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    ASSERT_TRUE(conn.Execute("CREATE TABLE \"odd name\" (\"a b\" TEXT);").ok());

    auto rowid = InsertRow(conn, "odd name", {{"a b", Text("x")}});
    ASSERT_TRUE(rowid.ok());
    ASSERT_TRUE(
        UpdateCell(conn, "odd name", rowid.value(), "a b", Text("y")).ok());
    EXPECT_EQ(Scalar(conn, "SELECT \"a b\" FROM \"odd name\";"), "y");
}

}  // namespace
}  // namespace sqlite_manager_gui
