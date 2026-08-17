#include "table_view.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>

#include "sqlite_manager/query_result.h"

namespace sqlite_manager_cli {
namespace {

using sqlite_manager::Cell;
using sqlite_manager::QueryResult;
using sqlite_manager::ValueType;

// The view renders a model directly, with no database involved.

Cell Txt(std::string text) { return {ValueType::kText, std::move(text)}; }
Cell Int(std::string text) { return {ValueType::kInteger, std::move(text)}; }
Cell Null() { return {ValueType::kNull, {}}; }

std::string RenderTable(const QueryResult& result) {
    std::ostringstream out;
    TableView().Write(result, out);
    return out.str();
}

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

}  // namespace
}  // namespace sqlite_manager_cli
