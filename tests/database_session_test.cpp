#include "gui/core/database_session.h"

#include <gtest/gtest.h>

#include <utility>

#include "sqlite_manager/query_result.h"

namespace sqlite_manager_gui {
namespace {

DatabaseSession OpenMemory() {
    auto session = DatabaseSession::Open(":memory:");
    EXPECT_TRUE(session.ok());
    return std::move(session).value();
}

TEST(DatabaseSessionTest, OpensInMemoryDatabase) {
    auto session = DatabaseSession::Open(":memory:");
    ASSERT_TRUE(session.ok());
    EXPECT_TRUE(session.value().IsOpen());
    EXPECT_EQ(session.value().path(), ":memory:");
}

TEST(DatabaseSessionTest, OpeningAMissingFileReadOnlyFails) {
    auto session = DatabaseSession::Open("/no/such/path.db",
                                         DatabaseSession::OpenMode::kReadOnly);
    EXPECT_FALSE(session.ok());
}

TEST(DatabaseSessionTest, ExecutesAndQueries) {
    auto session = OpenMemory();
    ASSERT_TRUE(
        session.Execute("CREATE TABLE t (id INTEGER, name TEXT);").ok());
    ASSERT_TRUE(
        session.Execute("INSERT INTO t VALUES (1, 'a'), (2, NULL);").ok());

    auto r = session.RunQuery("SELECT id, name FROM t ORDER BY id;");
    ASSERT_TRUE(r.ok());
    const auto& qr = r.value();
    ASSERT_EQ(qr.rows.size(), 2U);
    EXPECT_EQ(qr.rows[0][0].text, "1");
    EXPECT_EQ(qr.rows[1][1].type, sqlite_manager::ValueType::kNull);
}

TEST(DatabaseSessionTest, SurfacesSchemaThroughTheFacade) {
    auto session = OpenMemory();
    ASSERT_TRUE(
        session.Execute("CREATE TABLE t (id INTEGER PRIMARY KEY);").ok());

    auto objects = session.ListObjects();
    ASSERT_TRUE(objects.ok());
    ASSERT_EQ(objects.value().size(), 1U);
    EXPECT_EQ(objects.value()[0].name, "t");

    auto table = session.DescribeTable("t");
    ASSERT_TRUE(table.ok());
    ASSERT_EQ(table.value().columns.size(), 1U);
    EXPECT_TRUE(table.value().columns[0].primary_key);
}

TEST(DatabaseSessionTest, ReportsQueryErrors) {
    auto session = OpenMemory();
    EXPECT_FALSE(session.RunQuery("SELECT * FROM missing;").ok());
}

}  // namespace
}  // namespace sqlite_manager_gui
