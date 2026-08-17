#include "gui/gtk/result_grid.h"

#include <giomm/liststore.h>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <gtkmm/columnview.h>
#include <gtkmm/columnviewcolumn.h>
#include <gtkmm/editablelabel.h>
#include <gtkmm/label.h>
#include <gtkmm/listitem.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/singleselection.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sqlite_manager_gui::gtk {

namespace {

// One result row, wrapped as a GObject so it can live in a Gio::ListStore.
// Carries its rowid (0 when the grid is read-only) for edit addressing.
class RowObject : public Glib::Object {
public:
    static Glib::RefPtr<RowObject> create(std::int64_t rowid,
                                          std::vector<std::string> cells) {
        return Glib::make_refptr_for_instance<RowObject>(
            new RowObject(rowid, std::move(cells)));
    }

    std::int64_t rowid() const { return rowid_; }
    const std::vector<std::string>& cells() const { return cells_; }
    void set_cell(std::size_t i, std::string text) {
        if (i < cells_.size()) cells_[i] = std::move(text);
    }

protected:
    RowObject(std::int64_t rowid, std::vector<std::string> cells)
        : Glib::ObjectBase(typeid(RowObject)),
          rowid_(rowid),
          cells_(std::move(cells)) {}

private:
    std::int64_t rowid_;
    std::vector<std::string> cells_;
};

// Display text of a cell: its value, or empty for SQL NULL.
std::string CellText(const sqlite_manager::Cell& cell) {
    return cell.type == sqlite_manager::ValueType::kNull ? std::string()
                                                         : cell.text;
}

}  // namespace

ResultGrid::ResultGrid() {
    set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    view_ = Gtk::make_managed<Gtk::ColumnView>();
    set_child(*view_);
}

void ResultGrid::RemoveColumns() {
    auto columns = view_->get_columns();
    for (guint i = columns->get_n_items(); i > 0; --i) {
        auto column = std::dynamic_pointer_cast<Gtk::ColumnViewColumn>(
            columns->get_object(i - 1));
        if (column) view_->remove_column(column);
    }
}

bool ResultGrid::AcceptEdit(std::int64_t rowid, std::size_t column,
                            const std::string& text) {
    if (column >= columns_.size() || !edit_handler_) return false;
    return edit_handler_(rowid, columns_[column], text);
}

void ResultGrid::SetResult(const sqlite_manager::QueryResult& result) {
    Populate(result, /*editable=*/false, {});
}

void ResultGrid::SetEditableResult(const sqlite_manager::QueryResult& result,
                                   std::vector<std::int64_t> rowids) {
    Populate(result, /*editable=*/true, rowids);
}

void ResultGrid::Populate(const sqlite_manager::QueryResult& result,
                          bool editable,
                          const std::vector<std::int64_t>& rowids) {
    RemoveColumns();
    edit_conns_.clear();
    columns_ = result.columns;

    auto store = Gio::ListStore<RowObject>::create();
    for (std::size_t r = 0; r < result.rows.size(); ++r) {
        std::vector<std::string> cells;
        cells.reserve(result.rows[r].size());
        for (const auto& cell : result.rows[r]) cells.push_back(CellText(cell));
        const std::int64_t rowid =
            (editable && r < rowids.size()) ? rowids[r] : 0;
        store->append(RowObject::create(rowid, std::move(cells)));
    }
    view_->set_model(Gtk::SingleSelection::create(store));

    for (std::size_t c = 0; c < result.columns.size(); ++c) {
        auto factory = Gtk::SignalListItemFactory::create();

        if (editable) {
            factory->signal_setup().connect(
                [](const Glib::RefPtr<Gtk::ListItem>& item) {
                    auto* cell = Gtk::make_managed<Gtk::EditableLabel>();
                    cell->set_halign(Gtk::Align::START);
                    cell->set_margin(4);
                    item->set_child(*cell);
                });
            factory->signal_bind().connect(
                [this, c](const Glib::RefPtr<Gtk::ListItem>& item) {
                    auto row =
                        std::dynamic_pointer_cast<RowObject>(item->get_item());
                    auto* cell =
                        dynamic_cast<Gtk::EditableLabel*>(item->get_child());
                    if (!row || !cell) return;
                    if (c < row->cells().size())
                        cell->set_text(row->cells()[c]);
                    edit_conns_[cell] =
                        cell->property_editing().signal_changed().connect(
                            [this, cell, c, row]() {
                                if (cell->property_editing().get_value())
                                    return;
                                const std::string old_text =
                                    c < row->cells().size() ? row->cells()[c]
                                                            : std::string();
                                const std::string new_text =
                                    cell->get_text().raw();
                                if (new_text == old_text) return;
                                if (AcceptEdit(row->rowid(), c, new_text)) {
                                    row->set_cell(c, new_text);
                                } else {
                                    cell->set_text(old_text);
                                }
                            });
                });
            factory->signal_unbind().connect(
                [this](const Glib::RefPtr<Gtk::ListItem>& item) {
                    auto* cell =
                        dynamic_cast<Gtk::EditableLabel*>(item->get_child());
                    if (!cell) return;
                    auto it = edit_conns_.find(cell);
                    if (it != edit_conns_.end()) {
                        it->second.disconnect();
                        edit_conns_.erase(it);
                    }
                });
        } else {
            factory->signal_setup().connect(
                [](const Glib::RefPtr<Gtk::ListItem>& item) {
                    auto* label = Gtk::make_managed<Gtk::Label>();
                    label->set_halign(Gtk::Align::START);
                    label->set_margin(4);
                    item->set_child(*label);
                });
            factory->signal_bind().connect(
                [c](const Glib::RefPtr<Gtk::ListItem>& item) {
                    auto row =
                        std::dynamic_pointer_cast<RowObject>(item->get_item());
                    auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
                    if (row && label && c < row->cells().size()) {
                        label->set_text(row->cells()[c]);
                    }
                });
        }

        auto column = Gtk::ColumnViewColumn::create(result.columns[c], factory);
        column->set_resizable(true);
        if (editable) column->set_expand(true);
        view_->append_column(column);
    }
}

void ResultGrid::Clear() {
    RemoveColumns();
    edit_conns_.clear();
    columns_.clear();
    view_->set_model(
        Gtk::SingleSelection::create(Gio::ListStore<RowObject>::create()));
}

}  // namespace sqlite_manager_gui::gtk
