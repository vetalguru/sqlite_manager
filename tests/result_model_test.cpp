#include "gui/wtl/result_model.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "sqlite_manager/query_result.h"
#include "sqlite_manager/statement.h"  // ValueType

namespace sqlite_manager_gui::wtl {
namespace {

using sqlite_manager::Cell;
using sqlite_manager::QueryResult;
using sqlite_manager::ValueType;

Cell MakeCell(ValueType type, std::string text) {
    Cell cell;
    cell.type = type;
    cell.text = std::move(text);
    return cell;
}

// ---------- CellText ----------

TEST(ResultModelCellText, NullRendersEmpty) {
    // NULL carries no text; even if it did, it renders empty.
    EXPECT_EQ(CellText(MakeCell(ValueType::kNull, "ignored")), "");
}

TEST(ResultModelCellText, ValuesPassThroughTheirText) {
    EXPECT_EQ(CellText(MakeCell(ValueType::kInteger, "42")), "42");
    EXPECT_EQ(CellText(MakeCell(ValueType::kFloat, "1.5")), "1.5");
    EXPECT_EQ(CellText(MakeCell(ValueType::kText, "hello")), "hello");
    EXPECT_EQ(CellText(MakeCell(ValueType::kBlob, "41")), "41");
}

TEST(ResultModelCellText, EmptyTextStaysEmpty) {
    // An empty string value and a NULL both display empty (as in CSV).
    EXPECT_EQ(CellText(MakeCell(ValueType::kText, "")), "");
}

// ---------- ColumnTitles ----------

TEST(ResultModelColumnTitles, ReturnsColumnsInOrder) {
    QueryResult result;
    result.columns = {"id", "name", "grams"};
    EXPECT_EQ(ColumnTitles(result),
              (std::vector<std::string>{"id", "name", "grams"}));
}

TEST(ResultModelColumnTitles, EmptyWhenNoColumns) {
    QueryResult result;
    EXPECT_TRUE(ColumnTitles(result).empty());
}

// ---------- RowText ----------

TEST(ResultModelRowText, MapsEachCellAndEmptiesNull) {
    QueryResult result;
    result.columns = {"a", "b", "c"};
    result.rows.push_back({MakeCell(ValueType::kInteger, "1"),
                           MakeCell(ValueType::kNull, ""),
                           MakeCell(ValueType::kText, "x")});
    EXPECT_EQ(RowText(result, 0), (std::vector<std::string>{"1", "", "x"}));
}

TEST(ResultModelRowText, OutOfRangeRowIsEmptyPaddedToColumns) {
    QueryResult result;
    result.columns = {"a", "b"};
    // No rows: any index is out of range and yields empty, column-sized.
    EXPECT_EQ(RowText(result, 0), (std::vector<std::string>{"", ""}));
    EXPECT_EQ(RowText(result, 5), (std::vector<std::string>{"", ""}));
}

TEST(ResultModelRowText, ShortRowIsPaddedToColumnCount) {
    QueryResult result;
    result.columns = {"a", "b", "c"};
    result.rows.push_back({MakeCell(ValueType::kText, "only-one")});
    EXPECT_EQ(RowText(result, 0),
              (std::vector<std::string>{"only-one", "", ""}));
}

}  // namespace
}  // namespace sqlite_manager_gui::wtl
