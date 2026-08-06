#include "sqlite_manager/statement.h"

#include <sqlite3.h>

#include <cstring>
#include <utility>

#include "sqlite_manager/connection.h"

namespace sqlite_manager {

namespace {

Error MakeError(sqlite3_stmt* stmt) {
    sqlite3* db = sqlite3_db_handle(stmt);
    return Error::FromSqlite(sqlite3_extended_errcode(db),
                             sqlite3_errmsg(db));
}

Error NotPrepared() {
    return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                 "statement is not prepared");
}

}  // namespace

Result<Statement> Statement::Prepare(Connection& conn,
                                     const std::string& sql) {
    if (!conn.IsOpen()) {
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "connection is not open");
    }

    sqlite3_stmt* stmt = nullptr;
    const char* tail = nullptr;
    const int rc = sqlite3_prepare_v2(conn.raw(), sql.c_str(),
                                      static_cast<int>(sql.size()) + 1,
                                      &stmt, &tail);

    if (rc != SQLITE_OK) {
        // stmt is guaranteed NULL on failure; error state is on the db.
        return Error::FromSqlite(sqlite3_extended_errcode(conn.raw()),
                                 sqlite3_errmsg(conn.raw()));
    }

    // sql was blank or contained only comments: nothing was compiled.
    if (stmt == nullptr) {
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "sql contains no statement");
    }

    // Enforce the single-statement contract: anything but trailing
    // whitespace after the first statement is a misuse.
    if (tail != nullptr) {
        for (const char* p = tail; *p != '\0'; ++p) {
            if (std::strchr(" \t\r\n", *p) == nullptr) {
                sqlite3_finalize(stmt);
                return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                             "sql contains more than one statement");
            }
        }
    }

    return Statement(stmt);
}

Statement::~Statement() {
    // sqlite3_finalize(nullptr) is a harmless no-op, but be explicit.
    if (stmt_ != nullptr) {
        sqlite3_finalize(stmt_);
    }
}

Statement::Statement(Statement&& other) noexcept
    : stmt_(std::exchange(other.stmt_, nullptr)) {}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (stmt_ != nullptr) {
            sqlite3_finalize(stmt_);
        }
        stmt_ = std::exchange(other.stmt_, nullptr);
    }
    return *this;
}

// --- Binding ---

Status Statement::BindInt64(int index, std::int64_t value) {
    if (stmt_ == nullptr) return NotPrepared();
    const int rc = sqlite3_bind_int64(stmt_, index, value);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

Status Statement::BindDouble(int index, double value) {
    if (stmt_ == nullptr) return NotPrepared();
    const int rc = sqlite3_bind_double(stmt_, index, value);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

Status Statement::BindText(int index, const std::string& value) {
    if (stmt_ == nullptr) return NotPrepared();
    // SQLITE_TRANSIENT: SQLite makes its own copy of the string,
    // so `value` may die before Step(). Safe default over lifetimes.
    const int rc = sqlite3_bind_text(stmt_, index, value.c_str(),
                                     static_cast<int>(value.size()),
                                     SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

Status Statement::BindBlob(int index, const void* data, int size) {
    if (stmt_ == nullptr) return NotPrepared();
    const int rc = sqlite3_bind_blob(stmt_, index, data, size,
                                     SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

Status Statement::BindNull(int index) {
    if (stmt_ == nullptr) return NotPrepared();
    const int rc = sqlite3_bind_null(stmt_, index);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

// --- Execution ---

Result<Statement::StepResult> Statement::Step() {
    if (stmt_ == nullptr) return NotPrepared();
    const int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) return StepResult::kRow;
    if (rc == SQLITE_DONE) return StepResult::kDone;
    return MakeError(stmt_);
}

Status Statement::Reset() {
    if (stmt_ == nullptr) return NotPrepared();
    // sqlite3_reset returns the error of the previous Step() if it
    // failed; that error was already reported, so we surface only
    // genuinely new failures. SQLITE_OK covers the normal case.
    const int rc = sqlite3_reset(stmt_);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

// --- Reading ---

int Statement::ColumnCount() const {
    if (stmt_ == nullptr) return 0;
    return sqlite3_column_count(stmt_);
}

std::int64_t Statement::ColumnInt64(int index) const {
    if (stmt_ == nullptr) return 0;
    return sqlite3_column_int64(stmt_, index);
}

double Statement::ColumnDouble(int index) const {
    if (stmt_ == nullptr) return 0.0;
    return sqlite3_column_double(stmt_, index);
}

std::string Statement::ColumnText(int index) const {
    if (stmt_ == nullptr) return {};
    const unsigned char* text = sqlite3_column_text(stmt_, index);
    if (text == nullptr) return {};   // NULL column
    const int size = sqlite3_column_bytes(stmt_, index);
    return std::string(reinterpret_cast<const char*>(text),
                       static_cast<std::size_t>(size));
}

std::vector<std::uint8_t> Statement::ColumnBlob(int index) const {
    if (stmt_ == nullptr) return {};
    const void* data = sqlite3_column_blob(stmt_, index);
    if (data == nullptr) return {};   // NULL or zero-length blob
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    const int size = sqlite3_column_bytes(stmt_, index);
    return std::vector<std::uint8_t>(bytes, bytes + size);
}

bool Statement::ColumnIsNull(int index) const {
    if (stmt_ == nullptr) return true;
    return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
}

}  // namespace sqlite_manager
