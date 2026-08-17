#include "gui/gtk/schema_sidebar.h"

#include <glibmm/markup.h>
#include <glibmm/ustring.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>

namespace sqlite_manager_gui::gtk {

namespace {

const char* KindLabel(ObjectKind kind) {
    switch (kind) {
        case ObjectKind::kTable:
            return "table";
        case ObjectKind::kView:
            return "view";
        case ObjectKind::kIndex:
            return "index";
        case ObjectKind::kTrigger:
            return "trigger";
    }
    return "";
}

}  // namespace

SchemaSidebar::SchemaSidebar() {
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    list_ = Gtk::make_managed<Gtk::ListBox>();
    list_->set_selection_mode(Gtk::SelectionMode::SINGLE);
    set_child(*list_);
}

void SchemaSidebar::Clear() {
    while (auto* row = list_->get_row_at_index(0)) {
        list_->remove(*row);
    }
}

void SchemaSidebar::Show(const std::vector<ObjectInfo>& objects) {
    Clear();
    for (const auto& object : objects) {
        auto* label = Gtk::make_managed<Gtk::Label>();
        label->set_markup(Glib::Markup::escape_text(object.name) +
                          "  <span alpha='55%' size='small'>" +
                          KindLabel(object.kind) + "</span>");
        label->set_halign(Gtk::Align::START);
        label->set_margin(6);
        list_->append(*label);
    }
}

}  // namespace sqlite_manager_gui::gtk
