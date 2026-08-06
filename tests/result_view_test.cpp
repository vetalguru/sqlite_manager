#include "result_view.h"

#include <gtest/gtest.h>

#include <optional>
#include <sstream>
#include <string>

#include "query_result.h"

namespace sqlite_manager_cli {
namespace {

// The View renders a model directly, with no database involved.

std::string Render(const QueryResult& result) {
    std::ostringstream out;
    TableView().Render(result, out);
    return out.str();
}

TEST(TableViewTest, RendersHeaderRuleAndRows) {
    QueryResult r;
    r.columns = {"id", "name"};
    r.rows = {
        {std::string("1"), std::string("M855")},
        {std::string("2"), std::string("SMK 175")},
    };
    EXPECT_EQ(Render(r),
              "| id | name    |\n"
              "|----|---------|\n"
              "| 1  | M855    |\n"
              "| 2  | SMK 175 |\n");
}

TEST(TableViewTest, WidthTracksTheWidestCellNotJustTheHeader) {
    QueryResult r;
    r.columns = {"x"};
    r.rows = {{std::string("longer")}};
    EXPECT_EQ(Render(r),
              "| x      |\n"
              "|--------|\n"
              "| longer |\n");
}

TEST(TableViewTest, RendersNullCellAsNull) {
    QueryResult r;
    r.columns = {"v"};
    r.rows = {{std::nullopt}};
    EXPECT_EQ(Render(r),
              "| v    |\n"
              "|------|\n"
              "| NULL |\n");
}

TEST(TableViewTest, EmptyResultPrintsHeaderAndRuleOnly) {
    QueryResult r;
    r.columns = {"a", "bb"};
    EXPECT_EQ(Render(r),
              "| a | bb |\n"
              "|---|----|\n");
}

}  // namespace
}  // namespace sqlite_manager_cli
