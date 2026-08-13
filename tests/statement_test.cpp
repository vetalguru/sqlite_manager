#include "sqlite_manager/statement.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "sqlite_manager/connection.h"

namespace sqlite_manager {
namespace {

using StepResult = Statement::StepResult;

class StatementTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(conn_.Open(":memory:").ok());
        ASSERT_TRUE(conn_.Execute(
            "CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, score REAL);"
            "INSERT INTO t (name, score) VALUES ('alpha', 1.5), ('beta', 2.5);"
        ).ok());
    }

    Connection conn_;
};

// ---------- Prepare ----------

TEST_F(StatementTest, PreparesValidSql) {
    auto stmt = Statement::Prepare(conn_, "SELECT * FROM t");
    ASSERT_TRUE(stmt.ok()) << stmt.error().message;
    EXPECT_TRUE(stmt.value().IsValid());
}

TEST_F(StatementTest, PrepareInvalidSqlFails) {
    auto stmt = Statement::Prepare(conn_, "SELEKT * FROM t");
    ASSERT_FALSE(stmt.ok());
    EXPECT_EQ(stmt.error().code, ErrorCode::kError);
    EXPECT_FALSE(stmt.error().message.empty());
}

TEST_F(StatementTest, PrepareUnknownTableFails) {
    auto stmt = Statement::Prepare(conn_, "SELECT * FROM no_such_table");
    ASSERT_FALSE(stmt.ok());
    EXPECT_EQ(stmt.error().code, ErrorCode::kError);
}

TEST_F(StatementTest, PrepareOnClosedConnectionFails) {
    Connection closed;
    auto stmt = Statement::Prepare(closed, "SELECT 1");
    ASSERT_FALSE(stmt.ok());
    EXPECT_EQ(stmt.error().code, ErrorCode::kMisuse);
}

TEST_F(StatementTest, PrepareEmptySqlFails) {
    auto stmt = Statement::Prepare(conn_, "   -- just a comment");
    ASSERT_FALSE(stmt.ok());
    EXPECT_EQ(stmt.error().code, ErrorCode::kMisuse);
}

TEST_F(StatementTest, PrepareRejectsMultipleStatements) {
    auto stmt = Statement::Prepare(conn_, "SELECT 1; SELECT 2");
    ASSERT_FALSE(stmt.ok());
    EXPECT_EQ(stmt.error().code, ErrorCode::kMisuse);
}

TEST_F(StatementTest, PrepareAllowsTrailingSemicolonAndWhitespace) {
    auto stmt = Statement::Prepare(conn_, "SELECT 1;  \n\t ");
    ASSERT_TRUE(stmt.ok()) << stmt.error().message;
}

// ---------- Step / Column reads ----------

TEST_F(StatementTest, ReadsAllRowsAndColumns) {
    auto stmt = Statement::Prepare(
        conn_, "SELECT id, name, score FROM t ORDER BY id");
    ASSERT_TRUE(stmt.ok());
    Statement& s = stmt.value();

    EXPECT_EQ(s.ColumnCount(), 3);

    auto step = s.Step();
    ASSERT_TRUE(step.ok());
    ASSERT_EQ(step.value(), StepResult::kRow);
    EXPECT_EQ(s.ColumnInt64(0), 1);
    EXPECT_EQ(s.ColumnText(1), "alpha");
    EXPECT_DOUBLE_EQ(s.ColumnDouble(2), 1.5);

    step = s.Step();
    ASSERT_TRUE(step.ok());
    ASSERT_EQ(step.value(), StepResult::kRow);
    EXPECT_EQ(s.ColumnText(1), "beta");

    step = s.Step();
    ASSERT_TRUE(step.ok());
    EXPECT_EQ(step.value(), StepResult::kDone);
}

TEST_F(StatementTest, ColumnNamesComeFromSql) {
    auto stmt = Statement::Prepare(
        conn_, "SELECT id, name AS title, score * 2 AS doubled FROM t");
    ASSERT_TRUE(stmt.ok());
    Statement& s = stmt.value();
    EXPECT_EQ(s.ColumnName(0), "id");
    EXPECT_EQ(s.ColumnName(1), "title");
    EXPECT_EQ(s.ColumnName(2), "doubled");
}

TEST_F(StatementTest, ColumnIsNullDetectsNull) {
    ASSERT_TRUE(conn_.Execute("INSERT INTO t (name) VALUES (NULL)").ok());
    auto stmt = Statement::Prepare(
        conn_, "SELECT name, score FROM t WHERE name IS NULL");
    ASSERT_TRUE(stmt.ok());
    Statement& s = stmt.value();

    auto step = s.Step();
    ASSERT_TRUE(step.ok());
    ASSERT_EQ(step.value(), StepResult::kRow);
    EXPECT_TRUE(s.ColumnIsNull(0));
    EXPECT_TRUE(s.ColumnIsNull(1));   // score not set either
    EXPECT_EQ(s.ColumnText(0), "");   // NULL reads as empty string
}

// ---------- Binding ----------

TEST_F(StatementTest, BindsAllTypes) {
    ASSERT_TRUE(conn_.Execute(
        "CREATE TABLE bt (i INTEGER, d REAL, s TEXT, b BLOB, n TEXT)").ok());

    auto insert = Statement::Prepare(
        conn_, "INSERT INTO bt VALUES (?, ?, ?, ?, ?)");
    ASSERT_TRUE(insert.ok());
    Statement& ins = insert.value();

    const std::vector<std::uint8_t> blob = {0x01, 0x00, 0xFF};
    ASSERT_TRUE(ins.BindInt64(1, INT64_C(9007199254740993)).ok());
    ASSERT_TRUE(ins.BindDouble(2, 3.25).ok());
    ASSERT_TRUE(ins.BindText(3, "hello").ok());
    ASSERT_TRUE(ins.BindBlob(4, blob.data(),
                             static_cast<int>(blob.size())).ok());
    ASSERT_TRUE(ins.BindNull(5).ok());

    auto step = ins.Step();
    ASSERT_TRUE(step.ok()) << step.error().message;
    EXPECT_EQ(step.value(), StepResult::kDone);

    auto select = Statement::Prepare(conn_, "SELECT * FROM bt");
    ASSERT_TRUE(select.ok());
    Statement& sel = select.value();
    ASSERT_EQ(sel.Step().value(), StepResult::kRow);
    EXPECT_EQ(sel.ColumnInt64(0), INT64_C(9007199254740993));
    EXPECT_DOUBLE_EQ(sel.ColumnDouble(1), 3.25);
    EXPECT_EQ(sel.ColumnText(2), "hello");
    EXPECT_EQ(sel.ColumnBlob(3), blob);
    EXPECT_TRUE(sel.ColumnIsNull(4));
}

TEST_F(StatementTest, BindTextSurvivesSourceDestruction) {
    auto stmt = Statement::Prepare(conn_, "SELECT ?");
    ASSERT_TRUE(stmt.ok());
    Statement& s = stmt.value();

    {
        std::string temp = "short-lived";
        ASSERT_TRUE(s.BindText(1, temp).ok());
    }   // temp destroyed before Step: SQLITE_TRANSIENT must protect us

    ASSERT_EQ(s.Step().value(), StepResult::kRow);
    EXPECT_EQ(s.ColumnText(0), "short-lived");
}

TEST_F(StatementTest, BindTextWithEmbeddedNul) {
    auto stmt = Statement::Prepare(conn_, "SELECT ?");
    ASSERT_TRUE(stmt.ok());
    Statement& s = stmt.value();

    const std::string with_nul("ab\0cd", 5);
    ASSERT_TRUE(s.BindText(1, with_nul).ok());
    ASSERT_EQ(s.Step().value(), StepResult::kRow);
    EXPECT_EQ(s.ColumnText(0), with_nul);   // size-based read keeps the NUL
}

TEST_F(StatementTest, BindOutOfRangeIndexFails) {
    auto stmt = Statement::Prepare(conn_, "SELECT ?");
    ASSERT_TRUE(stmt.ok());

    const Status s = stmt.value().BindInt64(2, 1);   // only 1 parameter
    ASSERT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kRange);
}

// ---------- Reset ----------

TEST_F(StatementTest, ResetAllowsReExecution) {
    auto stmt = Statement::Prepare(
        conn_, "SELECT COUNT(*) FROM t WHERE id >= ?");
    ASSERT_TRUE(stmt.ok());
    Statement& s = stmt.value();

    ASSERT_TRUE(s.BindInt64(1, 1).ok());
    ASSERT_EQ(s.Step().value(), StepResult::kRow);
    EXPECT_EQ(s.ColumnInt64(0), 2);

    ASSERT_TRUE(s.Reset().ok());
    ASSERT_TRUE(s.BindInt64(1, 2).ok());
    ASSERT_EQ(s.Step().value(), StepResult::kRow);
    EXPECT_EQ(s.ColumnInt64(0), 1);
}

// ---------- Empty / moved-from state ----------

TEST_F(StatementTest, DefaultConstructedIsInvalid) {
    Statement s;
    EXPECT_FALSE(s.IsValid());
    EXPECT_EQ(s.BindInt64(1, 1).error().code, ErrorCode::kMisuse);
    EXPECT_FALSE(s.Step().ok());
    EXPECT_EQ(s.ColumnCount(), 0);
    EXPECT_EQ(s.ColumnName(0), "");
    EXPECT_TRUE(s.ColumnIsNull(0));
}

TEST_F(StatementTest, MoveTransfersOwnership) {
    auto prepared = Statement::Prepare(conn_, "SELECT 42");
    ASSERT_TRUE(prepared.ok());

    Statement a = std::move(prepared).value();
    ASSERT_TRUE(a.IsValid());

    Statement b(std::move(a));
    EXPECT_TRUE(b.IsValid());
    EXPECT_FALSE(a.IsValid());   // donor left empty

    ASSERT_EQ(b.Step().value(), StepResult::kRow);
    EXPECT_EQ(b.ColumnInt64(0), 42);
}

TEST_F(StatementTest, StatementFinalizedBeforeConnectionCloses) {
    Connection conn;
    ASSERT_TRUE(conn.Open(":memory:").ok());
    {
        auto stmt = Statement::Prepare(conn, "SELECT 1");
        ASSERT_TRUE(stmt.ok());
        // Close while a statement is alive: SQLite reports kBusy.
        const Status close_status = conn.Close();
        ASSERT_FALSE(close_status.ok());
        EXPECT_EQ(close_status.error().code, ErrorCode::kBusy);
    }   // statement finalized here
    EXPECT_TRUE(conn.Close().ok());
}

}  // namespace
}  // namespace sqlite_manager