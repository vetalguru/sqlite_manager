#include "gui/core/schema_editor.h"

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

Connection MakeDb() {
    Connection conn;
    EXPECT_TRUE(conn.Open(":memory:").ok());
    EXPECT_TRUE(
        conn.Execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT);")
            .ok());
    return conn;
}

TEST(SchemaEditorTest, AddColumnAddsANullableColumn) {
    auto conn = MakeDb();
    ASSERT_TRUE(AddColumn(conn, "t", "age", "INTEGER").ok());
    EXPECT_EQ(Scalar(conn,
                     "SELECT count(*) FROM pragma_table_info('t') "
                     "WHERE name = 'age';"),
              "1");
}

TEST(SchemaEditorTest, DropColumnRemovesIt) {
    auto conn = MakeDb();
    ASSERT_TRUE(DropColumn(conn, "t", "name").ok());
    EXPECT_EQ(Scalar(conn,
                     "SELECT count(*) FROM pragma_table_info('t') "
                     "WHERE name = 'name';"),
              "0");
}

TEST(SchemaEditorTest, DropColumnOnPrimaryKeyFails) {
    auto conn = MakeDb();
    EXPECT_FALSE(DropColumn(conn, "t", "id").ok());
}

TEST(SchemaEditorTest, CreateTableBuildsColumnsAndConstraints) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    const std::vector<ColumnDef> columns = {
        {"id", "INTEGER", false, true},
        {"name", "TEXT", true, false},
    };
    ASSERT_TRUE(CreateTable(conn, "u", columns).ok());
    EXPECT_EQ(Scalar(conn,
                     "SELECT count(*) FROM sqlite_master "
                     "WHERE type = 'table' AND name = 'u';"),
              "1");
    // The NOT NULL constraint is enforced.
    EXPECT_FALSE(conn.Execute("INSERT INTO u (id) VALUES (1);").ok());
}

TEST(SchemaEditorTest, CreateTableRequiresAName) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    EXPECT_FALSE(CreateTable(conn, "", {{"a", "TEXT", false, false}}).ok());
}

TEST(SchemaEditorTest, CreateTableRequiresColumns) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    EXPECT_FALSE(CreateTable(conn, "u", {}).ok());
}

TEST(SchemaEditorTest, DropTableRemovesIt) {
    auto conn = MakeDb();
    ASSERT_TRUE(DropTable(conn, "t").ok());
    EXPECT_EQ(
        Scalar(conn, "SELECT count(*) FROM sqlite_master WHERE name = 't';"),
        "0");
}

TEST(SchemaEditorTest, QuotesIdentifiers) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    ASSERT_TRUE(
        CreateTable(conn, "odd name", {{"a b", "TEXT", false, false}}).ok());
    ASSERT_TRUE(AddColumn(conn, "odd name", "c d", "INTEGER").ok());
    EXPECT_EQ(
        Scalar(conn, "SELECT count(*) FROM pragma_table_info('odd name');"),
        "2");
}

}  // namespace
}  // namespace sqlite_manager_gui
