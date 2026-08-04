#include "sqlite_manager/version.h"

#include <gtest/gtest.h>
#include <sqlite3.h>

#include <string>

namespace sqlite_manager {
namespace {

TEST(VersionTest, SqliteVersionIsNotEmpty) {
    const std::string version = SqliteVersion();
    EXPECT_FALSE(version.empty());
    // Format is "X.Y.Z", so at least 5 characters and starts with major "3".
    EXPECT_GE(version.size(), 5U);
    EXPECT_EQ(version[0], '3');
}

TEST(VersionTest, RuntimeVersionMatchesCompiledHeader) {
    // Catches the classic mistake: compiled against one sqlite3.h,
    // linked against a different sqlite3 library.
    EXPECT_EQ(SqliteVersionNumber(), SQLITE_VERSION_NUMBER);
    EXPECT_STREQ(SqliteVersion(), SQLITE_VERSION);
}

TEST(VersionTest, VersionNumberIsConsistentWithString) {
    // SQLITE_VERSION_NUMBER = major*1000000 + minor*1000 + patch
    const int number = SqliteVersionNumber();
    EXPECT_EQ(number / 1000000, 3);
    EXPECT_GT(number, 3000000);
}

}  // namespace
}  // namespace sqlite_manager
