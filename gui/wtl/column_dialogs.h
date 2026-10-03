#ifndef SQLITE_MANAGER_GUI_WTL_COLUMN_DIALOGS_H
#define SQLITE_MANAGER_GUI_WTL_COLUMN_DIALOGS_H

#include <string>
#include <utility>
#include <vector>

#include "gui/wtl/resource.h"
#include "gui/wtl/wtl.h"

namespace sqlite_manager_gui::wtl {

namespace detail {

// Reads a control's full text into a std::wstring.
inline std::wstring ControlText(const CWindow& parent, int id) {
    CWindow control = parent.GetDlgItem(id);
    const int length = control.GetWindowTextLength();
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    const int copied = control.GetWindowText(text.data(), length + 1);
    text.resize(static_cast<size_t>(copied));
    return text;
}

}  // namespace detail

// Collects a new column's name and (optional) declared type.
class CAddColumnDialog : public CDialogImpl<CAddColumnDialog> {
public:
    enum { IDD = IDD_ADD_COLUMN };

    std::wstring name;
    std::wstring type;

    // clang-format off
    BEGIN_MSG_MAP(CAddColumnDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()
    // clang-format on

    LRESULT OnOK(WORD /*code*/, WORD /*id*/, HWND /*ctl*/, BOOL& /*handled*/) {
        name = detail::ControlText(*this, IDC_COL_NAME);
        type = detail::ControlText(*this, IDC_COL_TYPE);
        EndDialog(IDOK);
        return 0;
    }

    LRESULT OnCancel(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                     BOOL& /*handled*/) {
        EndDialog(IDCANCEL);
        return 0;
    }
};

// Picks one of a table's columns to drop.
class CDropColumnDialog : public CDialogImpl<CDropColumnDialog> {
public:
    enum { IDD = IDD_DROP_COLUMN };

    explicit CDropColumnDialog(std::vector<std::wstring> columns)
        : columns_(std::move(columns)) {}

    std::wstring selected;

    // clang-format off
    BEGIN_MSG_MAP(CDropColumnDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()
    // clang-format on

    BOOL OnInitDialog(HWND /*focus*/, LPARAM /*param*/) {
        CComboBox combo = GetDlgItem(IDC_COL_LIST);
        for (const std::wstring& column : columns_) {
            combo.AddString(column.c_str());
        }
        if (!columns_.empty()) combo.SetCurSel(0);
        return TRUE;
    }

    LRESULT OnOK(WORD /*code*/, WORD /*id*/, HWND /*ctl*/, BOOL& /*handled*/) {
        CComboBox combo = GetDlgItem(IDC_COL_LIST);
        const int sel = combo.GetCurSel();
        if (sel >= 0 && static_cast<size_t>(sel) < columns_.size()) {
            selected = columns_[static_cast<size_t>(sel)];
        }
        EndDialog(IDOK);
        return 0;
    }

    LRESULT OnCancel(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                     BOOL& /*handled*/) {
        EndDialog(IDCANCEL);
        return 0;
    }

private:
    std::vector<std::wstring> columns_;
};

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_COLUMN_DIALOGS_H
