#include "gui/gtk/application.h"

#include "gui/gtk/main_window.h"

namespace sqlite_manager_gui::gtk {

Application::Application() : Gtk::Application("org.sqlite_manager.Gui") {}

Glib::RefPtr<Application> Application::create() {
    return Glib::make_refptr_for_instance<Application>(new Application());
}

void Application::on_activate() {
    auto* window = create_window();
    window->present();
}

MainWindow* Application::create_window() {
    auto* window = new MainWindow();
    // Hand the window to the application, and delete it once it closes.
    add_window(*window);
    window->signal_hide().connect([window]() { delete window; });
    return window;
}

}  // namespace sqlite_manager_gui::gtk
