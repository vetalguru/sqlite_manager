#include "gui/core/query_runner.h"

#include <gtest/gtest.h>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/query_result.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager_gui {
namespace {

sqlite_manager::Connection OpenMemory() {
    sqlite_manager::Connection conn;
    EXPECT_TRUE(conn.Open(":memory:").ok());
    return conn;
}

TEST(QueryRunnerTest, CollectsColumnsWithTheirStorageTypes) {
    auto conn = OpenMemory();
    auto r = RunSql(conn, "SELECT 1 AS i, 2.5 AS r, 'x' AS s, NULL AS n;");
    ASSERT_TRUE(r.ok());

    const auto& qr = r.value();
    ASSERT_EQ(qr.columns.size(), 4U);
    EXPECT_EQ(qr.columns[0], "i");
    ASSERT_EQ(qr.rows.size(), 1U);
    EXPECT_EQ(qr.rows[0][0].type, sqlite_manager::ValueType::kInteger);
    EXPECT_EQ(qr.rows[0][0].text, "1");
    EXPECT_EQ(qr.rows[0][1].type, sqlite_manager::ValueType::kFloat);
    EXPECT_EQ(qr.rows[0][2].type, sqlite_manager::ValueType::kText);
    EXPECT_EQ(qr.rows[0][3].type, sqlite_manager::ValueType::kNull);
    EXPECT_TRUE(qr.rows[0][3].text.empty());
}

TEST(QueryRunnerTest, NonQueryStatementYieldsEmptyResult) {
    auto conn = OpenMemory();
    ASSERT_TRUE(RunSql(conn, "CREATE TABLE t (x);").ok());

    auto r = RunSql(conn, "INSERT INTO t VALUES (1);");
    ASSERT_TRUE(r.ok());
    EXPECT_TRUE(r.value().columns.empty());
    EXPECT_TRUE(r.value().rows.empty());
}

TEST(QueryRunnerTest, ReportsPrepareErrors) {
    auto conn = OpenMemory();
    EXPECT_FALSE(RunSql(conn, "SELECT * FROM does_not_exist;").ok());
}

}  // namespace
}  // namespace sqlite_manager_gui
