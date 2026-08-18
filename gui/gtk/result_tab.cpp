#include "gui/gtk/result_tab.h"

#include <gtkmm/enums.h>

#include <utility>

#include "gui/gtk/result_grid.h"

namespace sqlite_manager_gui::gtk {

ResultTab::ResultTab(std::string key)
    : Gtk::Box(Gtk::Orientation::VERTICAL), key_(std::move(key)) {
    grid_ = Gtk::make_managed<ResultGrid>();
    grid_->set_vexpand(true);
    append(*grid_);
}

}  // namespace sqlite_manager_gui::gtk
