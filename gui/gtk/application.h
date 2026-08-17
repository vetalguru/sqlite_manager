#ifndef SQLITE_MANAGER_GUI_GTK_APPLICATION_H
#define SQLITE_MANAGER_GUI_GTK_APPLICATION_H

#include <glibmm/refptr.h>
#include <gtkmm/application.h>

namespace sqlite_manager_gui::gtk {

class MainWindow;

// The GTK front-end's Gtk::Application: owns the top-level window and the
// application lifecycle. All database work is delegated to the toolkit-
// free core (gui/core), so this layer stays thin - and so a Qt or other
// front-end can reuse the same core unchanged.
class Application : public Gtk::Application {
public:
    static Glib::RefPtr<Application> create();

protected:
    Application();

    void on_activate() override;

private:
    MainWindow* create_window();
};

}  // namespace sqlite_manager_gui::gtk

#endif  // SQLITE_MANAGER_GUI_GTK_APPLICATION_H
