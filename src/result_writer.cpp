#include "sqlite_manager/result_writer.h"

#include <array>
#include <cstddef>
#include <cstdio>
#include <ostream>
#include <string>

namespace sqlite_manager {

namespace {

// RFC 4180 field: quote it when it contains a comma, double quote, CR or
// LF; escape embedded quotes by doubling them.
std::string CsvField(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) {
        return value;
    }
    std::string out = "\"";
    for (const char c : value) {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
}

// A quoted, escaped JSON string per RFC 8259.
std::string JsonString(const std::string& value) {
    std::string out = "\"";
    for (const char ch : value) {
        const auto c = static_cast<unsigned char>(ch);
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            default:
                if (c < 0x20) {
                    std::array<char, 7> buf{};
                    std::snprintf(buf.data(), buf.size(), "\\u%04x", c);
                    out += buf.data();
                } else {
                    out += ch;
                }
        }
    }
    out += '"';
    return out;
}

// True when `text` is a number per the RFC 8259 grammar:
//   -? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)?
// SQLite renders infinities as "Inf" / "-Inf", which JSON cannot express.
bool IsJsonNumber(const std::string& text) {
    std::size_t i = 0;
    const std::size_t n = text.size();
    auto digits = [&]() {
        const std::size_t start = i;
        while (i < n && text[i] >= '0' && text[i] <= '9') ++i;
        return i > start;
    };
    if (i < n && text[i] == '-') ++i;
    if (i < n && text[i] == '0') {
        ++i;
    } else if (!digits()) {
        return false;
    }
    if (i < n && text[i] == '.') {
        ++i;
        if (!digits()) return false;
    }
    if (i < n && (text[i] == 'e' || text[i] == 'E')) {
        ++i;
        if (i < n && (text[i] == '+' || text[i] == '-')) ++i;
        if (!digits()) return false;
    }
    return i == n;
}

// A blob's bytes as a quoted lowercase hex string: always valid JSON,
// whatever the bytes are.
std::string JsonHex(const std::string& bytes) {
    static constexpr std::array<char, 16> kHex = {'0', '1', '2', '3', '4', '5',
                                                  '6', '7', '8', '9', 'a', 'b',
                                                  'c', 'd', 'e', 'f'};
    std::string out = "\"";
    out.reserve(bytes.size() * 2 + 2);
    for (const char ch : bytes) {
        const auto c = static_cast<unsigned char>(ch);
        out += kHex[c >> 4U];
        out += kHex[c & 0x0FU];
    }
    out += '"';
    return out;
}

}  // namespace

void CsvWriter::Write(const QueryResult& result, std::ostream& out) const {
    const std::size_t columns = result.columns.size();

    for (std::size_t i = 0; i < columns; ++i) {
        if (i > 0) out << ',';
        out << CsvField(result.columns[i]);
    }
    out << '\n';

    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < columns; ++i) {
            if (i > 0) out << ',';
            const Cell& cell = row[i];
            // SQL NULL becomes an empty field.
            out << CsvField(cell.type == ValueType::kNull ? std::string()
                                                          : cell.text);
        }
        out << '\n';
    }
}

void JsonWriter::Write(const QueryResult& result, std::ostream& out) const {
    const std::size_t columns = result.columns.size();

    out << '[';
    for (std::size_t r = 0; r < result.rows.size(); ++r) {
        out << (r == 0 ? "\n" : ",\n") << "  {";
        const auto& row = result.rows[r];
        for (std::size_t i = 0; i < columns; ++i) {
            if (i > 0) out << ", ";
            out << JsonString(result.columns[i]) << ": ";
            const Cell& cell = row[i];
            switch (cell.type) {
                case ValueType::kNull:
                    out << "null";
                    break;
                case ValueType::kInteger:
                case ValueType::kFloat:
                    // SQLite's text form of a finite number is a valid JSON
                    // number; infinities have no JSON form and become null.
                    out << (IsJsonNumber(cell.text) ? cell.text : "null");
                    break;
                case ValueType::kText:
                    out << JsonString(cell.text);
                    break;
                case ValueType::kBlob:
                    out << JsonHex(cell.text);
                    break;
            }
        }
        out << '}';
    }
    out << (result.rows.empty() ? "]\n" : "\n]\n");
}

}  // namespace sqlite_manager
