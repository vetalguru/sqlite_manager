#include "sqlite_manager/error.h"

#include <sqlite3.h>

namespace sqlite_manager {

// Compile-time guarantee that our enum stays in sync with SQLite.
// If SQLite ever changes a result code value, the build fails here.
static_assert(static_cast<int>(ErrorCode::kOk) == SQLITE_OK);
static_assert(static_cast<int>(ErrorCode::kError) == SQLITE_ERROR);
static_assert(static_cast<int>(ErrorCode::kInternal) == SQLITE_INTERNAL);
static_assert(static_cast<int>(ErrorCode::kPerm) == SQLITE_PERM);
static_assert(static_cast<int>(ErrorCode::kAbort) == SQLITE_ABORT);
static_assert(static_cast<int>(ErrorCode::kBusy) == SQLITE_BUSY);
static_assert(static_cast<int>(ErrorCode::kLocked) == SQLITE_LOCKED);
static_assert(static_cast<int>(ErrorCode::kNoMem) == SQLITE_NOMEM);
static_assert(static_cast<int>(ErrorCode::kReadOnly) == SQLITE_READONLY);
static_assert(static_cast<int>(ErrorCode::kInterrupt) == SQLITE_INTERRUPT);
static_assert(static_cast<int>(ErrorCode::kIoErr) == SQLITE_IOERR);
static_assert(static_cast<int>(ErrorCode::kCorrupt) == SQLITE_CORRUPT);
static_assert(static_cast<int>(ErrorCode::kNotFound) == SQLITE_NOTFOUND);
static_assert(static_cast<int>(ErrorCode::kFull) == SQLITE_FULL);
static_assert(static_cast<int>(ErrorCode::kCantOpen) == SQLITE_CANTOPEN);
static_assert(static_cast<int>(ErrorCode::kProtocol) == SQLITE_PROTOCOL);
static_assert(static_cast<int>(ErrorCode::kEmpty) == SQLITE_EMPTY);
static_assert(static_cast<int>(ErrorCode::kSchema) == SQLITE_SCHEMA);
static_assert(static_cast<int>(ErrorCode::kTooBig) == SQLITE_TOOBIG);
static_assert(static_cast<int>(ErrorCode::kConstraint) == SQLITE_CONSTRAINT);
static_assert(static_cast<int>(ErrorCode::kMismatch) == SQLITE_MISMATCH);
static_assert(static_cast<int>(ErrorCode::kMisuse) == SQLITE_MISUSE);
static_assert(static_cast<int>(ErrorCode::kNoLfs) == SQLITE_NOLFS);
static_assert(static_cast<int>(ErrorCode::kAuth) == SQLITE_AUTH);
static_assert(static_cast<int>(ErrorCode::kFormat) == SQLITE_FORMAT);
static_assert(static_cast<int>(ErrorCode::kRange) == SQLITE_RANGE);
static_assert(static_cast<int>(ErrorCode::kNotADb) == SQLITE_NOTADB);
static_assert(static_cast<int>(ErrorCode::kNotice) == SQLITE_NOTICE);
static_assert(static_cast<int>(ErrorCode::kWarning) == SQLITE_WARNING);
static_assert(static_cast<int>(ErrorCode::kRow) == SQLITE_ROW);
static_assert(static_cast<int>(ErrorCode::kDone) == SQLITE_DONE);

ErrorCode ToErrorCode(int sqlite_code) {
    // SQLITE_ROW / SQLITE_DONE are not "errors" but step() outcomes;
    // they do not fit the primary-code masking below, so handle first.
    if (sqlite_code == SQLITE_ROW) return ErrorCode::kRow;
    if (sqlite_code == SQLITE_DONE) return ErrorCode::kDone;

    // Extended result codes carry the primary code in the low 8 bits,
    // e.g. SQLITE_BUSY_SNAPSHOT (517) & 0xFF == SQLITE_BUSY (5).
    const int primary = sqlite_code & 0xFF;
    if (primary >= SQLITE_OK && primary <= SQLITE_WARNING) {
        return static_cast<ErrorCode>(primary);
    }
    return ErrorCode::kUnknown;
}

const char* ErrorCodeName(ErrorCode code) {
    switch (code) {
        case ErrorCode::kOk:         return "kOk";
        case ErrorCode::kError:      return "kError";
        case ErrorCode::kInternal:   return "kInternal";
        case ErrorCode::kPerm:       return "kPerm";
        case ErrorCode::kAbort:      return "kAbort";
        case ErrorCode::kBusy:       return "kBusy";
        case ErrorCode::kLocked:     return "kLocked";
        case ErrorCode::kNoMem:      return "kNoMem";
        case ErrorCode::kReadOnly:   return "kReadOnly";
        case ErrorCode::kInterrupt:  return "kInterrupt";
        case ErrorCode::kIoErr:      return "kIoErr";
        case ErrorCode::kCorrupt:    return "kCorrupt";
        case ErrorCode::kNotFound:   return "kNotFound";
        case ErrorCode::kFull:       return "kFull";
        case ErrorCode::kCantOpen:   return "kCantOpen";
        case ErrorCode::kProtocol:   return "kProtocol";
        case ErrorCode::kEmpty:      return "kEmpty";
        case ErrorCode::kSchema:     return "kSchema";
        case ErrorCode::kTooBig:     return "kTooBig";
        case ErrorCode::kConstraint: return "kConstraint";
        case ErrorCode::kMismatch:   return "kMismatch";
        case ErrorCode::kMisuse:     return "kMisuse";
        case ErrorCode::kNoLfs:      return "kNoLfs";
        case ErrorCode::kAuth:       return "kAuth";
        case ErrorCode::kFormat:     return "kFormat";
        case ErrorCode::kRange:      return "kRange";
        case ErrorCode::kNotADb:     return "kNotADb";
        case ErrorCode::kNotice:     return "kNotice";
        case ErrorCode::kWarning:    return "kWarning";
        case ErrorCode::kRow:        return "kRow";
        case ErrorCode::kDone:       return "kDone";
        case ErrorCode::kUnknown:    return "kUnknown";
    }
    return "kUnknown";  // unreachable, silences -Wreturn-type
}

}  // namespace sqlite_manager
