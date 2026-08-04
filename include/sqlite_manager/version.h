#ifndef SQLITE_MANAGER_VERSION_H
#define SQLITE_MANAGER_VERSION_H

namespace sqlite_manager {

// Version of the sqlite_manager library itself.
inline constexpr int kVersionMajor = 0;
inline constexpr int kVersionMinor = 1;
inline constexpr int kVersionPatch = 0;

// Version of the bundled SQLite, queried at runtime.
// Returns a string like "3.50.4".
const char* SqliteVersion();

// Returns SQLITE_VERSION_NUMBER, e.g. 3050004.
int SqliteVersionNumber();

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_VERSION_H