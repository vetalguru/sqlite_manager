#include "sqlite_manager/transaction.h"

#include <utility>

#include "sqlite_manager/connection.h"

namespace sqlite_manager {

Result<Transaction> Transaction::Begin(Connection& conn) {
    // Execute() itself reports kMisuse on a closed connection, and
    // SQLite reports an error if a transaction is already active.
    const Status s = conn.Execute("BEGIN");
    if (!s.ok()) {
        return s.error();
    }
    return Transaction(&conn);
}

Transaction::~Transaction() {
    // Ignore the result: destructors cannot report failures.
    if (conn_ != nullptr) {
        static_cast<void>(conn_->Execute("ROLLBACK"));
    }
}

Transaction::Transaction(Transaction&& other) noexcept
    : conn_(std::exchange(other.conn_, nullptr)) {}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (conn_ != nullptr) {
            static_cast<void>(conn_->Execute("ROLLBACK"));
        }
        conn_ = std::exchange(other.conn_, nullptr);
    }
    return *this;
}

Status Transaction::Commit() {
    if (conn_ == nullptr) {
        return Error(ErrorCode::kMisuse, 21 /* SQLITE_MISUSE */,
                     "transaction is not active");
    }
    Status s = conn_->Execute("COMMIT");
    if (s.ok()) {
        conn_ = nullptr;  // deactivate: destructor must not roll back
    }
    return s;
}

Status Transaction::Rollback() {
    if (conn_ == nullptr) {
        return Error(ErrorCode::kMisuse, 21 /* SQLITE_MISUSE */,
                     "transaction is not active");
    }
    Status s = conn_->Execute("ROLLBACK");
    conn_ = nullptr;  // deactivate regardless: nothing left to guard
    return s;
}

}  // namespace sqlite_manager