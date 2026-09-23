#include "sqlite_manager/statement.h"

#include <sqlite3.h>

#include <climits>
#include <cstddef>
#include <utility>

#include "sqlite_manager/connection.h"

namespace sqlite_manager {

namespace {

Error MakeError(sqlite3_stmt* stmt) {
    sqlite3* db = sqlite3_db_handle(stmt);
    return Error::FromSqlite(sqlite3_extended_errcode(db), sqlite3_errmsg(db));
}

Error NotPrepared() {
    return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                 "statement is not prepared");
}

Error UnknownParameter(const std::string& name) {
    return Error(ErrorCode::kRange, SQLITE_RANGE,
                 "no such bind parameter: " + name);
}

}  // namespace

Result<Statement> Statement::PrepareNext(Connection& conn,
                                         const std::string& sql,
                                         std::size_t& pos) {
    if (!conn.IsOpen()) {
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "connection is not open");
    }

    // A blank, comment-only or ";" prefix compiles to a NULL statement
    // with the tail advanced past it: keep going until a real statement
    // (or the end of the text) turns up.
    while (pos < sql.size()) {
        const std::size_t remaining = sql.size() - pos;
        if (remaining >= static_cast<std::size_t>(INT_MAX)) {
            return Error(ErrorCode::kTooBig, SQLITE_TOOBIG, "sql is too long");
        }

        sqlite3_stmt* stmt = nullptr;
        const char* tail = nullptr;
        const char* const begin = sql.c_str() + pos;
        // The +1 includes the terminating NUL, which lets SQLite skip a
        // copy of the text.
        const int rc = sqlite3_prepare_v2(
            conn.raw(), begin, static_cast<int>(remaining) + 1, &stmt, &tail);

        if (rc != SQLITE_OK) {
            // stmt is guaranteed NULL on failure; error state is on the db.
            return Error::FromSqlite(sqlite3_extended_errcode(conn.raw()),
                                     sqlite3_errmsg(conn.raw()));
        }

        const auto consumed = static_cast<std::size_t>(tail - begin);
        pos += consumed;
        if (stmt != nullptr) return Statement(stmt);
        if (consumed == 0) break;  // no progress (embedded NUL): stop
    }

    pos = sql.size();
    return Statement();
}

Result<Statement> Statement::Prepare(Connection& conn, const std::string& sql) {
    std::size_t pos = 0;
    auto first = PrepareNext(conn, sql, pos);
    if (!first.ok()) return first;
    if (!first.value().IsValid()) {
        // sql was blank or contained only comments: nothing was compiled.
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "sql contains no statement");
    }

    // Enforce the single-statement contract: whitespace, comments and
    // empty statements may follow, another statement may not. Text that
    // does not even compile counts as one, too.
    auto extra = PrepareNext(conn, sql, pos);
    if (!extra.ok() || extra.value().IsValid()) {
        return Error(ErrorCode::kMisuse, SQLITE_MISUSE,
                     "sql contains more than one statement");
    }
    return first;
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
    const int rc =
        sqlite3_bind_text(stmt_, index, value.c_str(),
                          static_cast<int>(value.size()), SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

Status Statement::BindBlob(int index, const void* data, int size) {
    if (stmt_ == nullptr) return NotPrepared();
    const int rc =
        sqlite3_bind_blob(stmt_, index, data, size, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

Status Statement::BindNull(int index) {
    if (stmt_ == nullptr) return NotPrepared();
    const int rc = sqlite3_bind_null(stmt_, index);
    if (rc != SQLITE_OK) return MakeError(stmt_);
    return Ok();
}

// --- Binding by name (resolves the name, then binds by position) ---

Status Statement::BindInt64(const std::string& name, std::int64_t value) {
    if (stmt_ == nullptr) return NotPrepared();
    const int index = sqlite3_bind_parameter_index(stmt_, name.c_str());
    if (index == 0) return UnknownParameter(name);
    return BindInt64(index, value);
}

Status Statement::BindDouble(const std::string& name, double value) {
    if (stmt_ == nullptr) return NotPrepared();
    const int index = sqlite3_bind_parameter_index(stmt_, name.c_str());
    if (index == 0) return UnknownParameter(name);
    return BindDouble(index, value);
}

Status Statement::BindText(const std::string& name, const std::string& value) {
    if (stmt_ == nullptr) return NotPrepared();
    const int index = sqlite3_bind_parameter_index(stmt_, name.c_str());
    if (index == 0) return UnknownParameter(name);
    return BindText(index, value);
}

Status Statement::BindBlob(const std::string& name, const void* data,
                           int size) {
    if (stmt_ == nullptr) return NotPrepared();
    const int index = sqlite3_bind_parameter_index(stmt_, name.c_str());
    if (index == 0) return UnknownParameter(name);
    return BindBlob(index, data, size);
}

Status Statement::BindNull(const std::string& name) {
    if (stmt_ == nullptr) return NotPrepared();
    const int index = sqlite3_bind_parameter_index(stmt_, name.c_str());
    if (index == 0) return UnknownParameter(name);
    return BindNull(index);
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

std::string Statement::ColumnName(int index) const {
    if (stmt_ == nullptr) return {};
    const char* name = sqlite3_column_name(stmt_, index);
    if (name == nullptr) return {};  // out of memory
    return name;
}

ValueType Statement::ColumnType(int index) const {
    if (stmt_ == nullptr) return ValueType::kNull;
    switch (sqlite3_column_type(stmt_, index)) {
        case SQLITE_INTEGER:
            return ValueType::kInteger;
        case SQLITE_FLOAT:
            return ValueType::kFloat;
        case SQLITE_TEXT:
            return ValueType::kText;
        case SQLITE_BLOB:
            return ValueType::kBlob;
        default:
            return ValueType::kNull;  // SQLITE_NULL
    }
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
    if (text == nullptr) return {};  // NULL column
    const int size = sqlite3_column_bytes(stmt_, index);
    return std::string(reinterpret_cast<const char*>(text),
                       static_cast<std::size_t>(size));
}

std::vector<std::uint8_t> Statement::ColumnBlob(int index) const {
    if (stmt_ == nullptr) return {};
    const void* data = sqlite3_column_blob(stmt_, index);
    if (data == nullptr) return {};  // NULL or zero-length blob
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    const int size = sqlite3_column_bytes(stmt_, index);
    return std::vector<std::uint8_t>(bytes, bytes + size);
}

bool Statement::ColumnIsNull(int index) const {
    if (stmt_ == nullptr) return true;
    return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
}

}  // namespace sqlite_manager
