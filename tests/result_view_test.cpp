#include "result_view.h"

#include <gtest/gtest.h>

#include <optional>
#include <sstream>
#include <string>

#include "query_result.h"

namespace sqlite_manager_cli {
namespace {

// The views render a model directly, with no database involved.

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

// ---------- TableView ----------

TEST(TableViewTest, RendersFramedHeaderAndRows) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {
        {std::string("1"), std::string("M855")},
        {std::string("2"), std::string("SMK 175")},
    };
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
    r.rows = {{std::string("longer")}};
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
    r.rows = {{std::nullopt}};
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

// ---------- CsvView ----------

TEST(CsvViewTest, RendersHeaderAndRows) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {
        {std::string("1"), std::string("M855")},
        {std::string("2"), std::string("SMK")},
    };
    EXPECT_EQ(RenderCsv(r), "id,name\n1,M855\n2,SMK\n");
}

TEST(CsvViewTest, QuotesFieldsWithCommaQuoteOrNewline) {
    QueryResult r;
    r.columns = {"a", "b"};
    r.rows = {
        {std::string("x,y"), std::string("he said \"hi\"")},
        {std::string("line1\nline2"), std::string("plain")},
    };
    EXPECT_EQ(RenderCsv(r),
              "a,b\n"
              "\"x,y\",\"he said \"\"hi\"\"\"\n"
              "\"line1\nline2\",plain\n");
}

TEST(CsvViewTest, NullBecomesEmptyField) {
    QueryResult r;
    r.columns = {"v", "w"};
    r.rows = {{std::nullopt, std::string("x")}};
    EXPECT_EQ(RenderCsv(r), "v,w\n,x\n");
}

TEST(CsvViewTest, EmptyResultPrintsHeaderOnly) {
    QueryResult r;
    r.columns = {"a", "b"};
    EXPECT_EQ(RenderCsv(r), "a,b\n");
}

// ---------- JsonView ----------

std::string RenderJson(const QueryResult& result) {
    std::ostringstream out;
    JsonView().Render(result, out);
    return out.str();
}

TEST(JsonViewTest, RendersArrayOfObjectsWithNull) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {
        {std::string("1"), std::string("M855")},
        {std::string("2"), std::nullopt},
    };
    EXPECT_EQ(RenderJson(r),
              "[\n"
              "  {\"id\": \"1\", \"name\": \"M855\"},\n"
              "  {\"id\": \"2\", \"name\": null}\n"
              "]\n");
}

TEST(JsonViewTest, EscapesStringValues) {
    QueryResult r;
    r.columns = {"k"};
    r.rows = {{std::string("a\"b\\c\n")}};
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
