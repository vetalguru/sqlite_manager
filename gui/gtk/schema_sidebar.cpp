#include "gui/gtk/schema_sidebar.h"

#include <glibmm/markup.h>
#include <glibmm/ustring.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/listboxrow.h>
#include <sigc++/functors/mem_fun.h>

#include <algorithm>
#include <cstddef>

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

// Display order: tables, then views, indexes, and triggers.
int KindRank(ObjectKind kind) {
    switch (kind) {
        case ObjectKind::kTable:
            return 0;
        case ObjectKind::kView:
            return 1;
        case ObjectKind::kIndex:
            return 2;
        case ObjectKind::kTrigger:
            return 3;
    }
    return 4;
}

}  // namespace

SchemaSidebar::SchemaSidebar() {
    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);

    list_ = Gtk::make_managed<Gtk::ListBox>();
    list_->set_selection_mode(Gtk::SelectionMode::SINGLE);
    list_->signal_row_selected().connect(
        sigc::mem_fun(*this, &SchemaSidebar::OnRowSelected));
    set_child(*list_);
}

void SchemaSidebar::Clear() {
    objects_.clear();
    while (auto* row = list_->get_row_at_index(0)) {
        list_->remove(*row);
    }
}

void SchemaSidebar::Show(const std::vector<ObjectInfo>& objects) {
    Clear();

    objects_ = objects;
    std::stable_sort(objects_.begin(), objects_.end(),
                     [](const ObjectInfo& a, const ObjectInfo& b) {
                         const int ra = KindRank(a.kind);
                         const int rb = KindRank(b.kind);
                         if (ra != rb) return ra < rb;
                         return a.name < b.name;
                     });

    for (const auto& object : objects_) {
        auto* label = Gtk::make_managed<Gtk::Label>();
        label->set_markup(Glib::Markup::escape_text(object.name) +
                          "  <span alpha='55%' size='small'>" +
                          KindLabel(object.kind) + "</span>");
        label->set_halign(Gtk::Align::START);
        label->set_margin(6);
        list_->append(*label);
    }
}

void SchemaSidebar::SelectObject(const std::string& name) {
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        if (objects_[i].name == name) {
            if (auto* row = list_->get_row_at_index(static_cast<int>(i))) {
                list_->select_row(*row);
            }
            return;
        }
    }
}

void SchemaSidebar::OnRowSelected(Gtk::ListBoxRow* row) {
    if (row == nullptr) return;
    const int index = row->get_index();
    if (index >= 0 && static_cast<std::size_t>(index) < objects_.size()) {
        signal_object_selected_.emit(objects_[static_cast<std::size_t>(index)]);
    }
}

}  // namespace sqlite_manager_gui::gtk
