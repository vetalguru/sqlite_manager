#ifndef SQLITE_MANAGER_GUI_GTK_SCHEMA_SIDEBAR_H
#define SQLITE_MANAGER_GUI_GTK_SCHEMA_SIDEBAR_H

#include <gtkmm/scrolledwindow.h>

#include <vector>

#include "gui/core/schema_info.h"

namespace Gtk {
class ListBox;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

// Sidebar listing the database's schema objects (tables, views, indexes,
// triggers). A thin view over the core's ObjectInfo: it only displays,
// with no knowledge of how the schema was read.
class SchemaSidebar : public Gtk::ScrolledWindow {
public:
    SchemaSidebar();

    // Replaces the list with the given objects (each row shows the name
    // and a dimmed kind label).
    void Show(const std::vector<ObjectInfo>& objects);

    // Empties the list.
    void Clear();

private:
    Gtk::ListBox* list_ = nullptr;
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_SCHEMA_SIDEBAR_H
