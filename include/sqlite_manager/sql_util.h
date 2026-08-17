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

// Quotes `name` as a SQLite identifier: wraps it in double quotes and
// doubles any embedded double quote. Use it to build SQL that references
// a table or column whose name may contain special characters, keywords,
// or quotes - never string-concatenate a raw identifier.
std::string QuoteIdentifier(const std::string& name);

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_SQL_UTIL_H
