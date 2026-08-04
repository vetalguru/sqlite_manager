#include "sqlite_manager/version.h"

#include <sqlite3.h>

namespace sqlite_manager {

// Returns the SQLite version string, e.g. "3.50.4".
const char* SqliteVersion() { return sqlite3_libversion(); }

// Returns the SQLite version number, e.g. 3050004.
int SqliteVersionNumber() { return sqlite3_libversion_number(); }

}  // namespace sqlite_manager