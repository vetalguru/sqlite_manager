#include "sqlite_manager/connection.h"

#include <gtest/gtest.h>
#include <sqlite3.h>

#include <utility>

namespace sqlite_manager {
namespace {

// ---------- Open / Close / IsOpen ----------

TEST(ConnectionTest, DefaultConstructedIsClosed) {
    const Connection conn;
    EXPECT_FALSE(conn.IsOpen());
    EXPECT_EQ(conn.raw(), nullptr);
}

TEST(ConnectionTest, OpensInMemoryDatabase) {
    Connection conn;
    const Status s = conn.Open(":memory:");
    ASSERT_TRUE(s.ok()) << s.error().message;
    EXPECT_TRUE(conn.IsOpen());
    EXPECT_NE(conn.raw(), nullptr);
}

TEST(ConnectionTest, OpenTwiceFailsWithMisuse) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());

    const Status s = conn.Open(":memory:");
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kMisuse);
    EXPECT_TRUE(conn.IsOpen());  // first connection unaffected
}

TEST(ConnectionTest, OpenNonexistentDirectoryFails) {
    Connection conn;
    const Status s = conn.Open("/nonexistent_dir_12345/db.sqlite");
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kCantOpen);
    EXPECT_FALSE(conn.IsOpen());
}

TEST(ConnectionTest, CloseIsIdempotent) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());

    EXPECT_TRUE(conn.Close().ok());
    EXPECT_FALSE(conn.IsOpen());
    EXPECT_TRUE(conn.Close().ok());  // second close: no-op, still ok
}

TEST(ConnectionTest, CanReopenAfterClose) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    ASSERT_TRUE(conn.Close().ok());
    EXPECT_TRUE(conn.Open(":memory:").ok());
    EXPECT_TRUE(conn.IsOpen());
}

// ---------- Execute ----------

TEST(ConnectionTest, ExecuteCreatesTableAndInserts) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());

    Status s = conn.Execute("CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT)");
    ASSERT_TRUE(s.ok()) << s.error().message;

    s = conn.Execute("INSERT INTO t (name) VALUES ('alpha'), ('beta')");
    ASSERT_TRUE(s.ok()) << s.error().message;
}

TEST(ConnectionTest, ExecuteMultipleStatementsInOneCall) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());

    const Status s = conn.Execute(
        "CREATE TABLE a (x INTEGER);"
        "CREATE TABLE b (y INTEGER);"
        "INSERT INTO a VALUES (1);");
    ASSERT_TRUE(s.ok()) << s.error().message;
}

TEST(ConnectionTest, ExecuteInvalidSqlFails) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());

    const Status s = conn.Execute("THIS IS NOT SQL");
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kError);
    EXPECT_FALSE(s.error().message.empty());
}

TEST(ConnectionTest, ExecuteOnClosedConnectionFailsWithMisuse) {
    Connection conn;
    const Status s = conn.Execute("SELECT 1");
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kMisuse);
}

TEST(ConnectionTest, ExecuteConstraintViolationMapsToConstraint) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    ASSERT_TRUE(conn.Execute(
        "CREATE TABLE t (id INTEGER PRIMARY KEY, v TEXT UNIQUE);"
        "INSERT INTO t (v) VALUES ('dup');").ok());

    const Status s = conn.Execute("INSERT INTO t (v) VALUES ('dup')");
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kConstraint);
}

// ---------- LastInsertRowId ----------

TEST(ConnectionTest, LastInsertRowIdTracksInserts) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    ASSERT_TRUE(conn.Execute(
        "CREATE TABLE t (id INTEGER PRIMARY KEY, v TEXT)").ok());

    ASSERT_TRUE(conn.Execute("INSERT INTO t (v) VALUES ('a')").ok());
    EXPECT_EQ(conn.LastInsertRowId(), 1);
    ASSERT_TRUE(conn.Execute("INSERT INTO t (v) VALUES ('b')").ok());
    EXPECT_EQ(conn.LastInsertRowId(), 2);
}

TEST(ConnectionTest, LastInsertRowIdIsZeroBeforeAnyInsert) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    EXPECT_EQ(conn.LastInsertRowId(), 0);
}

TEST(ConnectionTest, LastInsertRowIdOnClosedConnectionIsZero) {
    const Connection conn;
    EXPECT_EQ(conn.LastInsertRowId(), 0);
}

// ---------- Move semantics ----------

TEST(ConnectionTest, MoveConstructorTransfersOwnership) {
    Connection a;
    ASSERT_TRUE(a.Open(":memory:").ok());
    sqlite3* handle = a.raw();

    Connection b(std::move(a));
    EXPECT_TRUE(b.IsOpen());
    EXPECT_EQ(b.raw(), handle);   // same handle, new owner
    EXPECT_FALSE(a.IsOpen());     // donor is left closed
}

TEST(ConnectionTest, MoveAssignmentTransfersAndClosesOld) {
    Connection a;
    ASSERT_TRUE(a.Open(":memory:").ok());
    ASSERT_TRUE(a.Execute("CREATE TABLE t (x INTEGER)").ok());

    Connection b;
    ASSERT_TRUE(b.Open(":memory:").ok());

    b = std::move(a);             // b's old connection must be closed
    EXPECT_TRUE(b.IsOpen());
    EXPECT_FALSE(a.IsOpen());
    // The moved-in connection is fully functional:
    EXPECT_TRUE(b.Execute("INSERT INTO t VALUES (1)").ok());
}

TEST(ConnectionTest, SelfMoveAssignmentIsSafe) {
    Connection a;
    ASSERT_TRUE(a.Open(":memory:").ok());
    sqlite3* handle = a.raw();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
    a = std::move(a);              // intentional: tests the self-move guard
#pragma GCC diagnostic pop
    EXPECT_TRUE(a.IsOpen());
    EXPECT_EQ(a.raw(), handle);
}

// ---------- File-based database ----------

TEST(ConnectionTest, CreatesAndReopensFileDatabase) {
    const std::string path = ::testing::TempDir() + "conn_test.sqlite";
    std::remove(path.c_str());

    {
        Connection conn;
        ASSERT_TRUE(conn.Open(path).ok());
        ASSERT_TRUE(conn.Execute(
            "CREATE TABLE t (x INTEGER); INSERT INTO t VALUES (42);").ok());
    }   // destructor closes and flushes

    Connection conn;
    ASSERT_TRUE(conn.Open(path).ok());
    // Table must persist; inserting again proves the schema survived.
    EXPECT_TRUE(conn.Execute("INSERT INTO t VALUES (43)").ok());

    std::remove(path.c_str());
}

TEST(ConnectionTest, ReadOnlyModeRejectsWrites) {
    const std::string path = ::testing::TempDir() + "ro_test.sqlite";
    std::remove(path.c_str());

    {
        Connection rw;
        ASSERT_TRUE(rw.Open(path).ok());
        ASSERT_TRUE(rw.Execute("CREATE TABLE t (x INTEGER)").ok());
    }

    Connection ro;
    ASSERT_TRUE(ro.Open(path, Connection::OpenMode::kReadOnly).ok());

    const Status s = ro.Execute("INSERT INTO t VALUES (1)");
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kReadOnly);

    std::remove(path.c_str());
}

TEST(ConnectionTest, ReadOnlyModeFailsOnMissingFile) {
    const std::string path =
        ::testing::TempDir() + "no_such_file_98765.sqlite";
    std::remove(path.c_str());

    Connection conn;
    const Status s = conn.Open(path, Connection::OpenMode::kReadOnly);
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kCantOpen);
    EXPECT_FALSE(conn.IsOpen());
}

}  // namespace
}  // namespace sqlite_manager
