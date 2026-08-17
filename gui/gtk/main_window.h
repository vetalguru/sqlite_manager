#ifndef SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H
#define SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H

#include <gtkmm/applicationwindow.h>

namespace sqlite_manager_gui::gtk {

// The application's main window. For now a skeleton with a placeholder
// body; the schema sidebar, SQL editor, and result grid are layered on
// top in later changes.
class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_MAIN_WINDOW_H
