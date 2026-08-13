#include "result_view.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>

#include "query_result.h"

namespace sqlite_manager_cli {
namespace {

// The views render a model directly, with no database involved.

Cell Txt(std::string text)  { return {ValueType::kText, std::move(text)}; }
Cell Int(std::string text)  { return {ValueType::kInteger, std::move(text)}; }
Cell Real(std::string text) { return {ValueType::kFloat, std::move(text)}; }
Cell Null()                 { return {ValueType::kNull, {}}; }

std::string RenderTable(const QueryResult& result) {
    std::ostringstream out;
    TableView().Render(result, out);
    return out.str();
}

std::string RenderCsv(const QueryResult& result) {
    std::ostringstream out;
    CsvView().Render(result, out);
    return out.str();
}

std::string RenderJson(const QueryResult& result) {
    std::ostringstream out;
    JsonView().Render(result, out);
    return out.str();
}

// ---------- TableView (renders every cell as its text) ----------

TEST(TableViewTest, RendersFramedHeaderAndRows) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {{Int("1"), Txt("M855")}, {Int("2"), Txt("SMK 175")}};
    EXPECT_EQ(RenderTable(r),
              "+----+---------+\n"
              "| id | name    |\n"
              "+----+---------+\n"
              "| 1  | M855    |\n"
              "| 2  | SMK 175 |\n"
              "+----+---------+\n");
}

TEST(TableViewTest, WidthTracksTheWidestCellNotJustTheHeader) {
    QueryResult r;
    r.columns = {"x"};
    r.rows = {{Txt("longer")}};
    EXPECT_EQ(RenderTable(r),
              "+--------+\n"
              "| x      |\n"
              "+--------+\n"
              "| longer |\n"
              "+--------+\n");
}

TEST(TableViewTest, RendersNullCellAsNull) {
    QueryResult r;
    r.columns = {"v"};
    r.rows = {{Null()}};
    EXPECT_EQ(RenderTable(r),
              "+------+\n"
              "| v    |\n"
              "+------+\n"
              "| NULL |\n"
              "+------+\n");
}

TEST(TableViewTest, EmptyResultPrintsHeaderBetweenFrames) {
    QueryResult r;
    r.columns = {"a", "bb"};
    EXPECT_EQ(RenderTable(r),
              "+---+----+\n"
              "| a | bb |\n"
              "+---+----+\n"
              "+---+----+\n");
}

// ---------- CsvView (renders every cell as its text) ----------

TEST(CsvViewTest, RendersHeaderAndRows) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {{Int("1"), Txt("M855")}, {Int("2"), Txt("SMK")}};
    EXPECT_EQ(RenderCsv(r), "id,name\n1,M855\n2,SMK\n");
}

TEST(CsvViewTest, QuotesFieldsWithCommaQuoteOrNewline) {
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

TEST(CsvViewTest, NullBecomesEmptyField) {
    QueryResult r;
    r.columns = {"v", "w"};
    r.rows = {{Null(), Txt("x")}};
    EXPECT_EQ(RenderCsv(r), "v,w\n,x\n");
}

TEST(CsvViewTest, EmptyResultPrintsHeaderOnly) {
    QueryResult r;
    r.columns = {"a", "b"};
    EXPECT_EQ(RenderCsv(r), "a,b\n");
}

// ---------- JsonView (types drive quoting) ----------

TEST(JsonViewTest, NumbersUnquotedTextQuotedNullAsNull) {
    QueryResult r;
    r.columns = {"i", "r", "s", "n"};
    r.rows = {{Int("42"), Real("1.5"), Txt("hi"), Null()}};
    EXPECT_EQ(RenderJson(r),
              "[\n"
              "  {\"i\": 42, \"r\": 1.5, \"s\": \"hi\", \"n\": null}\n"
              "]\n");
}

TEST(JsonViewTest, RendersArrayOfObjects) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {{Int("1"), Txt("M855")}, {Int("2"), Null()}};
    EXPECT_EQ(RenderJson(r),
              "[\n"
              "  {\"id\": 1, \"name\": \"M855\"},\n"
              "  {\"id\": 2, \"name\": null}\n"
              "]\n");
}

TEST(JsonViewTest, EscapesStringValues) {
    QueryResult r;
    r.columns = {"k"};
    r.rows = {{Txt("a\"b\\c\n")}};
    EXPECT_EQ(RenderJson(r),
              "[\n"
              "  {\"k\": \"a\\\"b\\\\c\\n\"}\n"
              "]\n");
}

TEST(JsonViewTest, EmptyResultIsEmptyArray) {
    QueryResult r;
    r.columns = {"a", "b"};
    EXPECT_EQ(RenderJson(r), "[]\n");
}

}  // namespace
}  // namespace sqlite_manager_cli
