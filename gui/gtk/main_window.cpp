#include "gui/gtk/main_window.h"

#include <gtkmm/label.h>

namespace sqlite_manager_gui::gtk {

MainWindow::MainWindow() {
    set_title("SQLite Manager");
    set_default_size(960, 640);

    auto* placeholder =
        Gtk::make_managed<Gtk::Label>("Open a database to begin.");
    set_child(*placeholder);
}

}  // namespace sqlite_manager_gui::gtk
