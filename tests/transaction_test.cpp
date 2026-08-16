#include "sqlite_manager/transaction.h"

#include <gtest/gtest.h>

#include <utility>

#include "sqlite_manager/connection.h"
#include "sqlite_manager/statement.h"

namespace sqlite_manager {
namespace {

class TransactionTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(conn_.Open(":memory:").ok());
        ASSERT_TRUE(conn_.Execute("CREATE TABLE t (x INTEGER)").ok());
    }

    std::int64_t CountRows() {
        auto stmt = Statement::Prepare(conn_, "SELECT COUNT(*) FROM t");
        EXPECT_TRUE(stmt.ok());
        EXPECT_EQ(stmt.value().Step().value(), Statement::StepResult::kRow);
        return stmt.value().ColumnInt64(0);
    }

    Connection conn_;
};

TEST_F(TransactionTest, CommitPersistsChanges) {
    auto txn = Transaction::Begin(conn_);
    ASSERT_TRUE(txn.ok()) << txn.error().message;
    ASSERT_TRUE(txn.value().IsActive());

    ASSERT_TRUE(conn_.Execute("INSERT INTO t VALUES (1), (2)").ok());
    ASSERT_TRUE(txn.value().Commit().ok());
    EXPECT_FALSE(txn.value().IsActive());

    EXPECT_EQ(CountRows(), 2);
}

TEST_F(TransactionTest, DestructorRollsBack) {
    {
        auto txn = Transaction::Begin(conn_);
        ASSERT_TRUE(txn.ok());
        ASSERT_TRUE(conn_.Execute("INSERT INTO t VALUES (1)").ok());
    }  // no Commit: destructor must roll back

    EXPECT_EQ(CountRows(), 0);
}

TEST_F(TransactionTest, ExplicitRollbackDiscardsChanges) {
    auto txn = Transaction::Begin(conn_);
    ASSERT_TRUE(txn.ok());
    ASSERT_TRUE(conn_.Execute("INSERT INTO t VALUES (1)").ok());

    ASSERT_TRUE(txn.value().Rollback().ok());
    EXPECT_FALSE(txn.value().IsActive());
    EXPECT_EQ(CountRows(), 0);
}

TEST_F(TransactionTest, CommitAfterCommitFailsWithMisuse) {
    auto txn = Transaction::Begin(conn_);
    ASSERT_TRUE(txn.ok());
    ASSERT_TRUE(txn.value().Commit().ok());

    const Status s = txn.value().Commit();
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kMisuse);
}

TEST_F(TransactionTest, BeginOnClosedConnectionFails) {
    Connection closed;
    auto txn = Transaction::Begin(closed);
    ASSERT_FALSE(txn.ok());
    EXPECT_EQ(txn.error().code, ErrorCode::kMisuse);
}

TEST_F(TransactionTest, NestedBeginFails) {
    auto outer = Transaction::Begin(conn_);
    ASSERT_TRUE(outer.ok());

    auto inner = Transaction::Begin(conn_);
    ASSERT_FALSE(inner.ok());
    EXPECT_EQ(inner.error().code, ErrorCode::kError);

    // Outer transaction still works.
    ASSERT_TRUE(conn_.Execute("INSERT INTO t VALUES (1)").ok());
    ASSERT_TRUE(outer.value().Commit().ok());
    EXPECT_EQ(CountRows(), 1);
}

TEST_F(TransactionTest, MoveTransfersGuard) {
    auto begun = Transaction::Begin(conn_);
    ASSERT_TRUE(begun.ok());

    Transaction a = std::move(begun).value();
    ASSERT_TRUE(a.IsActive());

    Transaction b(std::move(a));
    EXPECT_TRUE(b.IsActive());
    EXPECT_FALSE(a.IsActive());  // donor deactivated: no double rollback

    ASSERT_TRUE(conn_.Execute("INSERT INTO t VALUES (1)").ok());
    ASSERT_TRUE(b.Commit().ok());
    EXPECT_EQ(CountRows(), 1);
}

TEST_F(TransactionTest, DefaultConstructedIsInactive) {
    Transaction txn;
    EXPECT_FALSE(txn.IsActive());
    EXPECT_EQ(txn.Commit().error().code, ErrorCode::kMisuse);
    EXPECT_EQ(txn.Rollback().error().code, ErrorCode::kMisuse);
}

}  // namespace
}  // namespace sqlite_manager