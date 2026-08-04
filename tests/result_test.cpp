#include "sqlite_manager/error.h"
#include "sqlite_manager/result.h"

#include <gtest/gtest.h>
#include <sqlite3.h>

#include <string>
#include <utility>

namespace sqlite_manager {
namespace {

// ---------- ErrorCode mapping ----------

TEST(ErrorCodeTest, MapsPrimaryCodes) {
    EXPECT_EQ(ToErrorCode(SQLITE_OK), ErrorCode::kOk);
    EXPECT_EQ(ToErrorCode(SQLITE_ERROR), ErrorCode::kError);
    EXPECT_EQ(ToErrorCode(SQLITE_BUSY), ErrorCode::kBusy);
    EXPECT_EQ(ToErrorCode(SQLITE_CONSTRAINT), ErrorCode::kConstraint);
    EXPECT_EQ(ToErrorCode(SQLITE_MISUSE), ErrorCode::kMisuse);
    EXPECT_EQ(ToErrorCode(SQLITE_WARNING), ErrorCode::kWarning);
}

TEST(ErrorCodeTest, MapsStepOutcomes) {
    EXPECT_EQ(ToErrorCode(SQLITE_ROW), ErrorCode::kRow);
    EXPECT_EQ(ToErrorCode(SQLITE_DONE), ErrorCode::kDone);
}

TEST(ErrorCodeTest, ReducesExtendedCodesToPrimary) {
    // Extended code = primary | (n << 8). Check against real constants.
    EXPECT_EQ(ToErrorCode(SQLITE_BUSY_SNAPSHOT), ErrorCode::kBusy);
    EXPECT_EQ(ToErrorCode(SQLITE_CONSTRAINT_UNIQUE), ErrorCode::kConstraint);
    EXPECT_EQ(ToErrorCode(SQLITE_IOERR_READ), ErrorCode::kIoErr);
    EXPECT_EQ(ToErrorCode(SQLITE_CANTOPEN_ISDIR), ErrorCode::kCantOpen);
}

TEST(ErrorCodeTest, UnknownForOutOfRange) {
    EXPECT_EQ(ToErrorCode(9999 << 8 | 0xFE), ErrorCode::kUnknown);
    EXPECT_EQ(ToErrorCode(0xFE), ErrorCode::kUnknown);
}

TEST(ErrorCodeTest, NamesAreConsistent) {
    EXPECT_STREQ(ErrorCodeName(ErrorCode::kBusy), "kBusy");
    EXPECT_STREQ(ErrorCodeName(ErrorCode::kDone), "kDone");
    EXPECT_STREQ(ErrorCodeName(ErrorCode::kUnknown), "kUnknown");
}

TEST(ErrorTest, FromSqliteCapturesEverything) {
    const Error e = Error::FromSqlite(SQLITE_CONSTRAINT_UNIQUE,
                                      "UNIQUE constraint failed: t.id");
    EXPECT_EQ(e.code, ErrorCode::kConstraint);
    EXPECT_EQ(e.sqlite_code, SQLITE_CONSTRAINT_UNIQUE);  // raw preserved
    EXPECT_EQ(e.message, "UNIQUE constraint failed: t.id");
}

// ---------- Result ----------

TEST(ResultTest, HoldsValue) {
    Result<int> r = 42;
    ASSERT_TRUE(r);
    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.value(), 42);
}

TEST(ResultTest, HoldsError) {
    Result<int> r = Error::FromSqlite(SQLITE_BUSY, "database is locked");
    ASSERT_FALSE(r);
    EXPECT_EQ(r.error().code, ErrorCode::kBusy);
    EXPECT_EQ(r.error().message, "database is locked");
}

TEST(ResultTest, ValueIsMutable) {
    Result<std::string> r = std::string("abc");
    r.value() += "def";
    EXPECT_EQ(r.value(), "abcdef");
}

TEST(ResultTest, MoveOutValue) {
    Result<std::string> r = std::string("payload");
    const std::string moved = std::move(r).value();
    EXPECT_EQ(moved, "payload");
}

TEST(ResultTest, StatusOk) {
    const Status s = Ok();
    EXPECT_TRUE(s.ok());
}

TEST(ResultTest, StatusError) {
    const Status s = Error::FromSqlite(SQLITE_CANTOPEN, "unable to open");
    EXPECT_FALSE(s.ok());
    EXPECT_EQ(s.error().code, ErrorCode::kCantOpen);
}

// Simulates a real call site: function returns Result, caller checks.
Result<int> ParsePositive(int x) {
    if (x <= 0) {
        return Error(ErrorCode::kMismatch, SQLITE_MISMATCH, "not positive");
    }
    return x;
}

TEST(ResultTest, TypicalCallSite) {
    auto good = ParsePositive(7);
    ASSERT_TRUE(good);
    EXPECT_EQ(good.value(), 7);

    auto bad = ParsePositive(-1);
    ASSERT_FALSE(bad);
    EXPECT_EQ(bad.error().code, ErrorCode::kMismatch);
}

// ---------- Assert semantics (debug builds only) ----------

#ifndef NDEBUG
TEST(ResultDeathTest, ValueOnErrorAsserts) {
    Result<int> r = Error::FromSqlite(SQLITE_ERROR, "boom");
    EXPECT_DEATH(static_cast<void>(r.value()), "value\\(\\) called on an error");
}

TEST(ResultDeathTest, ErrorOnValueAsserts) {
    Result<int> r = 1;
    EXPECT_DEATH(static_cast<void>(r.error()), "error\\(\\) called on a success");
}
#endif

}  // namespace
}  // namespace sqlite_manager
