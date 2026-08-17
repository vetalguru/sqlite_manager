#ifndef SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H
#define SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H

#include <glibmm/ustring.h>
#include <gtkmm/applicationwindow.h>

#include <optional>
#include <string>

#include "gui/core/database_session.h"

namespace sqlite_manager_gui::gtk {

class SchemaSidebar;

// The application's main window: a header bar with an "Open" action, a
// schema sidebar, and (for now) a placeholder body. It owns the open
// DatabaseSession and drives the views; the SQL editor and result grid
// are layered into the body in later changes.
class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();

private:
    void OnOpenClicked();
    void OpenDatabase(const std::string& path);
    void RefreshSchema();
    void ReportError(const Glib::ustring& message);

    std::optional<DatabaseSession> session_;
    SchemaSidebar* sidebar_ = nullptr;  // owned by the widget tree
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H
