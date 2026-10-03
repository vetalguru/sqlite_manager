#ifndef SQLITE_MANAGER_GUI_WTL_RESULT_PANE_H
#define SQLITE_MANAGER_GUI_WTL_RESULT_PANE_H

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "gui/wtl/result_model.h"
#include "gui/wtl/text.h"
#include "gui/wtl/wtl.h"
#include "sqlite_manager/query_result.h"

namespace sqlite_manager_gui::wtl {

// One result tab: a child window hosting a report list plus the state needed
// to act on it. It renders a QueryResult and reports a cell double-click to a
// callback; the owning frame performs the database work on the active pane.
// Because the list is this window's own child, its notifications arrive here
// (not at the frame, which the tab view would otherwise intercept).
class CResultPane : public CWindowImpl<CResultPane> {
public:
    // (pane, item index, subitem/column index)
    using EditHandler = std::function<void(CResultPane&, int, int)>;

    DECLARE_WND_CLASS_EX(nullptr, 0, COLOR_WINDOW)

    explicit CResultPane(std::string key) : key_(std::move(key)) {}

    const std::string& key() const { return key_; }
    const std::string& table() const { return table_; }
    bool editable() const { return editable_; }
    const sqlite_manager::QueryResult& result() const { return result_; }
    const std::vector<std::int64_t>& rowids() const { return rowids_; }
    int SelectedRow() const { return m_list.GetSelectedIndex(); }
    void set_edit_handler(EditHandler handler) {
        on_edit_ = std::move(handler);
    }

    // clang-format off
    BEGIN_MSG_MAP(CResultPane)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_SIZE(OnSize)
        NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
    END_MSG_MAP()
    // clang-format on

    LRESULT OnCreate(LPCREATESTRUCT /*create*/) {
        m_list.Create(*this, rcDefault, nullptr,
                      WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
                      WS_EX_CLIENTEDGE);
        m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT |
                                        LVS_EX_GRIDLINES);
        return 0;
    }

    void OnSize(UINT /*type*/, CSize size) {
        if (m_list.m_hWnd != nullptr) {
            m_list.MoveWindow(0, 0, size.cx, size.cy);
        }
    }

    LRESULT OnDblClick(int /*id*/, LPNMHDR header, BOOL& /*handled*/) {
        const LPNMITEMACTIVATE activate =
            reinterpret_cast<LPNMITEMACTIVATE>(header);
        if (on_edit_) on_edit_(*this, activate->iItem, activate->iSubItem);
        return 0;
    }

    // Replaces the displayed result and the state used to edit it. `table`
    // is empty for a read-only view or ad-hoc query; `rowids` is parallel to
    // the result rows for an editable table.
    void Show(sqlite_manager::QueryResult result,
              std::vector<std::int64_t> rowids, bool editable,
              std::string table) {
        result_ = std::move(result);
        rowids_ = std::move(rowids);
        editable_ = editable;
        table_ = std::move(table);
        Fill();
    }

private:
    void Fill() {
        m_list.DeleteAllItems();
        CHeaderCtrl header = m_list.GetHeader();
        for (int i = header.GetItemCount() - 1; i >= 0; --i) {
            m_list.DeleteColumn(i);
        }
        for (size_t c = 0; c < result_.columns.size(); ++c) {
            m_list.InsertColumn(static_cast<int>(c),
                                Widen(result_.columns[c]).c_str(), LVCFMT_LEFT,
                                140);
        }
        for (size_t r = 0; r < result_.rows.size(); ++r) {
            const std::vector<std::string> cells = RowText(result_, r);
            m_list.InsertItem(static_cast<int>(r),
                              cells.empty() ? L"" : Widen(cells[0]).c_str());
            for (size_t c = 1; c < cells.size(); ++c) {
                m_list.SetItemText(static_cast<int>(r), static_cast<int>(c),
                                   Widen(cells[c]).c_str());
            }
        }
    }

    std::string key_;
    std::string table_;
    sqlite_manager::QueryResult result_;
    std::vector<std::int64_t> rowids_;
    bool editable_ = false;
    CListViewCtrl m_list;
    EditHandler on_edit_;
};

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_RESULT_PANE_H
