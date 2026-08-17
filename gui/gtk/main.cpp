#include "gui/gtk/application.h"

int main(int argc, char* argv[]) {
    return sqlite_manager_gui::gtk::Application::create()->run(argc, argv);
}
