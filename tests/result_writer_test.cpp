#include "sqlite_manager/result_writer.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>

#include "sqlite_manager/query_result.h"

namespace sqlite_manager {
namespace {

// The writers serialize a model directly, with no database involved.

Cell Txt(std::string text) { return {ValueType::kText, std::move(text)}; }
Cell Int(std::string text) { return {ValueType::kInteger, std::move(text)}; }
Cell Real(std::string text) { return {ValueType::kFloat, std::move(text)}; }
Cell Null() { return {ValueType::kNull, {}}; }

std::string RenderCsv(const QueryResult& result) {
    std::ostringstream out;
    CsvWriter().Write(result, out);
    return out.str();
}

std::string RenderJson(const QueryResult& result) {
    std::ostringstream out;
    JsonWriter().Write(result, out);
    return out.str();
}

// ---------- CsvWriter (renders every cell as its text) ----------

TEST(CsvWriterTest, RendersHeaderAndRows) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {{Int("1"), Txt("M855")}, {Int("2"), Txt("SMK")}};
    EXPECT_EQ(RenderCsv(r), "id,name\n1,M855\n2,SMK\n");
}

TEST(CsvWriterTest, QuotesFieldsWithCommaQuoteOrNewline) {
    QueryResult r;
    r.columns = {"a", "b"};
    r.rows = {
        {Txt("x,y"), Txt("he said \"hi\"")},
        {Txt("line1\nline2"), Txt("plain")},
    };
    EXPECT_EQ(RenderCsv(r),
              "a,b\n"
              "\"x,y\",\"he said \"\"hi\"\"\"\n"
              "\"line1\nline2\",plain\n");
}

TEST(CsvWriterTest, NullBecomesEmptyField) {
    QueryResult r;
    r.columns = {"v", "w"};
    r.rows = {{Null(), Txt("x")}};
    EXPECT_EQ(RenderCsv(r), "v,w\n,x\n");
}

TEST(CsvWriterTest, EmptyResultPrintsHeaderOnly) {
    QueryResult r;
    r.columns = {"a", "b"};
    EXPECT_EQ(RenderCsv(r), "a,b\n");
}

// ---------- JsonWriter (types drive quoting) ----------

TEST(JsonWriterTest, NumbersUnquotedTextQuotedNullAsNull) {
    QueryResult r;
    r.columns = {"i", "r", "s", "n"};
    r.rows = {{Int("42"), Real("1.5"), Txt("hi"), Null()}};
    EXPECT_EQ(RenderJson(r),
              "[\n"
              "  {\"i\": 42, \"r\": 1.5, \"s\": \"hi\", \"n\": null}\n"
              "]\n");
}

TEST(JsonWriterTest, RendersArrayOfObjects) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {{Int("1"), Txt("M855")}, {Int("2"), Null()}};
    EXPECT_EQ(RenderJson(r),
              "[\n"
              "  {\"id\": 1, \"name\": \"M855\"},\n"
              "  {\"id\": 2, \"name\": null}\n"
              "]\n");
}

TEST(JsonWriterTest, EscapesStringValues) {
    QueryResult r;
    r.columns = {"k"};
    r.rows = {{Txt("a\"b\\c\n")}};
    EXPECT_EQ(RenderJson(r),
              "[\n"
              "  {\"k\": \"a\\\"b\\\\c\\n\"}\n"
              "]\n");
}

TEST(JsonWriterTest, EmptyResultIsEmptyArray) {
    QueryResult r;
    r.columns = {"a", "b"};
    EXPECT_EQ(RenderJson(r), "[]\n");
}

}  // namespace
}  // namespace sqlite_manager
