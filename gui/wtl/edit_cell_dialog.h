#ifndef SQLITE_MANAGER_GUI_WTL_EDIT_CELL_DIALOG_H
#define SQLITE_MANAGER_GUI_WTL_EDIT_CELL_DIALOG_H

#include <string>
#include <utility>

#include "gui/wtl/resource.h"
#include "gui/wtl/wtl.h"

namespace sqlite_manager_gui::wtl {

// A modal dialog that edits one cell's text. The caller seeds it with the
// column name (shown in the title) and the current value, and reads back
// Value() after DoModal() returns IDOK.
class CEditCellDialog : public CDialogImpl<CEditCellDialog> {
public:
    enum { IDD = IDD_EDIT_CELL };

    CEditCellDialog(std::wstring column, std::wstring value)
        : column_(std::move(column)), value_(std::move(value)) {}

    const std::wstring& Value() const { return value_; }

    // clang-format off
    BEGIN_MSG_MAP(CEditCellDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()
    // clang-format on

    BOOL OnInitDialog(HWND /*focus*/, LPARAM /*param*/) {
        SetWindowText((L"Edit " + column_).c_str());
        SetDlgItemText(IDC_EDIT_VALUE, value_.c_str());
        return TRUE;  // let the dialog set the initial focus
    }

    LRESULT OnOK(WORD /*code*/, WORD /*id*/, HWND /*ctl*/, BOOL& /*handled*/) {
        CWindow edit = GetDlgItem(IDC_EDIT_VALUE);
        const int length = edit.GetWindowTextLength();
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        const int copied = edit.GetWindowText(text.data(), length + 1);
        text.resize(static_cast<size_t>(copied));
        value_ = std::move(text);
        EndDialog(IDOK);
        return 0;
    }

    LRESULT OnCancel(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                     BOOL& /*handled*/) {
        EndDialog(IDCANCEL);
        return 0;
    }

private:
    std::wstring column_;
    std::wstring value_;
};

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_EDIT_CELL_DIALOG_H
