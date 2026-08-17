#ifndef SQLITE_MANAGER_SQL_UTIL_H
#define SQLITE_MANAGER_SQL_UTIL_H

#include <string>

namespace sqlite_manager {

// True when `sql` forms one or more complete statements: it ends in a
// semicolon that is not inside a string literal, a comment, or an
// unterminated trigger body (CREATE ... BEGIN ... END). Whitespace or a
// bare comment counts as incomplete.
//
// Wraps sqlite3_complete(). Use it to decide when accumulated REPL or
// script input is ready to execute - a hand-rolled "ends with ';'" check
// mis-splits triggers and semicolons inside strings.
bool IsCompleteStatement(const std::string& sql);

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_SQL_UTIL_H
