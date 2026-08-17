#include "gui/core/schema_reader.h"

#include <gtest/gtest.h>

#include "sqlite_manager/connection.h"

namespace sqlite_manager_gui {
namespace {

sqlite_manager::Connection MakeSchema() {
    sqlite_manager::Connection conn;
    EXPECT_TRUE(conn.Open(":memory:").ok());
    EXPECT_TRUE(conn.Execute("CREATE TABLE ammo (id INTEGER PRIMARY KEY, name "
                             "TEXT NOT NULL);"
                             "CREATE INDEX idx_ammo_name ON ammo(name);"
                             "CREATE VIEW v_ammo AS SELECT id FROM ammo;")
                    .ok());
    return conn;
}

TEST(SchemaReaderTest, ReadsUserObjectsByKind) {
    auto conn = MakeSchema();
    auto objects = ReadObjects(conn);
    ASSERT_TRUE(objects.ok());

    bool table = false;
    bool index = false;
    bool view = false;
    for (const auto& o : objects.value()) {
        if (o.kind == ObjectKind::kTable && o.name == "ammo") table = true;
        if (o.kind == ObjectKind::kIndex && o.name == "idx_ammo_name") {
            index = true;
        }
        if (o.kind == ObjectKind::kView && o.name == "v_ammo") view = true;
    }
    EXPECT_TRUE(table);
    EXPECT_TRUE(index);
    EXPECT_TRUE(view);
}

TEST(SchemaReaderTest, DescribesTableColumns) {
    auto conn = MakeSchema();
    auto table = ReadTable(conn, "ammo");
    ASSERT_TRUE(table.ok());

    EXPECT_FALSE(table.value().is_view);
    ASSERT_EQ(table.value().columns.size(), 2U);
    EXPECT_EQ(table.value().columns[0].name, "id");
    EXPECT_TRUE(table.value().columns[0].primary_key);
    EXPECT_EQ(table.value().columns[1].name, "name");
    EXPECT_TRUE(table.value().columns[1].not_null);
    EXPECT_FALSE(table.value().columns[1].primary_key);
}

TEST(SchemaReaderTest, FlagsViews) {
    auto conn = MakeSchema();
    auto view = ReadTable(conn, "v_ammo");
    ASSERT_TRUE(view.ok());
    EXPECT_TRUE(view.value().is_view);
}

}  // namespace
}  // namespace sqlite_manager_gui
