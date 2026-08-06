#ifndef SQLITE_MANAGER_RESULT_H
#define SQLITE_MANAGER_RESULT_H

#include <cassert>
#include <utility>
#include <variant>

#include "sqlite_manager/error.h"

namespace sqlite_manager {

template <typename T>
class [[nodiscard]] Result {
public:
    // Implicit by design (like absl::StatusOr): lets callers write
    // `return value;` or `return error;` from a Result-returning function.
    // NOLINTNEXTLINE(google-explicit-constructor)
    Result(T value) : data_(std::move(value)) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    Result(Error error) : data_(std::move(error)) {}

    bool ok() const { return std::holds_alternative<T>(data_); }
    explicit operator bool() const { return ok(); }

    T& value() & {
        assert(ok() && "Result::value() called on an error");
        return std::get<T>(data_);
    }
    const T& value() const& {
        assert(ok() && "Result::value() called on an error");
        return std::get<T>(data_);
    }
    // Allows `auto v = std::move(result).value();`
    T&& value() && {
        assert(ok() && "Result::value() called on an error");
        return std::get<T>(std::move(data_));
    }

    const Error& error() const {
        assert(!ok() && "Result::error() called on a success");
        return std::get<Error>(data_);
    }

private:
    std::variant<T, Error> data_;
};

// For operations that succeed or fail without producing a value
using Status = Result<std::monostate>;

// Readable success for Status-returning functions: return Ok()
inline Status Ok() { return Status(std::monostate{}); }

}  // namespace sqlite_manager

#endif  // SQLITE_MANAGER_RESULT_H
