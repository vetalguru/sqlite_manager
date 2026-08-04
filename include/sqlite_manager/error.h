#ifndef SQLITE_MANAGER_ERROR_H
#define SQLITE_MANAGER_ERROR_H

#include <string>

namespace sqlite_manager {

// Mirrors the full set of SQLite primary result codes.
// https://www.sqlite.org/rescode.html#primary_result_code_list
// Extended codes (e.g. SQLITE_BUSY_SNAPSHOT) map to their primary
// code here; the raw value is preserved in Error::sqlite_code.
// Values are kept in sync with sqlite3.h by static_asserts in error.cpp.
enum class ErrorCode {
    kOk = 0,        // SQLITE_OK         (0)  not an error
    kError,         // SQLITE_ERROR      (1)  generic error
    kInternal,      // SQLITE_INTERNAL   (2)  internal malfunction
    kPerm,          // SQLITE_PERM       (3)  access permission denied
    kAbort,         // SQLITE_ABORT      (4)  callback requested abort
    kBusy,          // SQLITE_BUSY       (5)  database file is locked
    kLocked,        // SQLITE_LOCKED     (6)  table is locked
    kNoMem,         // SQLITE_NOMEM      (7)  out of memory
    kReadOnly,      // SQLITE_READONLY   (8)  attempt to write readonly db
    kInterrupt,     // SQLITE_INTERRUPT  (9)  interrupted by sqlite3_interrupt
    kIoErr,         // SQLITE_IOERR      (10) disk I/O error
    kCorrupt,       // SQLITE_CORRUPT    (11) database image is malformed
    kNotFound,      // SQLITE_NOTFOUND   (12) unknown opcode (internal use)
    kFull,          // SQLITE_FULL       (13) database or disk is full
    kCantOpen,      // SQLITE_CANTOPEN   (14) unable to open database file
    kProtocol,      // SQLITE_PROTOCOL   (15) locking protocol problem
    kEmpty,         // SQLITE_EMPTY      (16) not used anymore
    kSchema,        // SQLITE_SCHEMA     (17) database schema changed
    kTooBig,        // SQLITE_TOOBIG     (18) string or blob exceeds limit
    kConstraint,    // SQLITE_CONSTRAINT (19) constraint violation
    kMismatch,      // SQLITE_MISMATCH   (20) data type mismatch
    kMisuse,        // SQLITE_MISUSE     (21) library used incorrectly
    kNoLfs,         // SQLITE_NOLFS      (22) no large file support
    kAuth,          // SQLITE_AUTH       (23) authorization denied
    kFormat,        // SQLITE_FORMAT     (24) not used anymore
    kRange,         // SQLITE_RANGE      (25) bind parameter out of range
    kNotADb,        // SQLITE_NOTADB     (26) file is not a database
    kNotice,        // SQLITE_NOTICE     (27) notification (logs only)
    kWarning,       // SQLITE_WARNING    (28) warning (logs only)
    kRow = 100,     // SQLITE_ROW        (100) step() has a row ready
    kDone = 101,    // SQLITE_DONE       (101) step() has finished
    kUnknown = -1   // any value we did not map
};

// Maps a raw SQLite result code (primary or extended) to ErrorCode.
// Extended codes are reduced to their primary code (low 8 bits).
ErrorCode ToErrorCode(int sqlite_code);

// Human-readable name of the enum value, e.g. "kBusy".
const char* ErrorCodeName(ErrorCode code);

struct Error {
    ErrorCode code = ErrorCode::kUnknown;
    int sqlite_code = 0;   // raw (possibly extended) SQLite code
    std::string message;   // captured from sqlite3_errmsg() at failure time

    Error() = default;
    Error(ErrorCode c, int raw_code, std::string msg)
        : code(c), sqlite_code(raw_code), message(std::move(msg)) {}

    // Convenience: build from a raw SQLite code.
    static Error FromSqlite(int sqlite_code, std::string message) {
        return Error(ToErrorCode(sqlite_code), sqlite_code, std::move(message));
    }
};

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_ERROR_H
