#ifndef SQLITE_MANAGER_GUI_GTK_SCHEMA_SIDEBAR_H
#define SQLITE_MANAGER_GUI_GTK_SCHEMA_SIDEBAR_H

#include <gtkmm/scrolledwindow.h>
#include <sigc++/signal.h>

#include <vector>

#include "gui/core/schema_info.h"

namespace Gtk {
class ListBox;
class ListBoxRow;
}  // namespace Gtk

namespace sqlite_manager_gui::gtk {

// Sidebar listing the database's schema objects, grouped tables first,
// then views, indexes, and triggers (name-ordered within each group). A
// thin view over the core's ObjectInfo: it only displays and reports the
// selected object.
class SchemaSidebar : public Gtk::ScrolledWindow {
public:
    SchemaSidebar();

    // Replaces the list with the given objects.
    void Show(const std::vector<ObjectInfo>& objects);

    // Empties the list.
    void Clear();

    // Emitted when the user selects an object row.
    sigc::signal<void(const ObjectInfo&)> signal_object_selected() {
        return signal_object_selected_;
    }

private:
    void OnRowSelected(Gtk::ListBoxRow* row);

    Gtk::ListBox* list_ = nullptr;
    std::vector<ObjectInfo> objects_;  // display order, indexed by row
    sigc::signal<void(const ObjectInfo&)> signal_object_selected_;
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_SCHEMA_SIDEBAR_H
