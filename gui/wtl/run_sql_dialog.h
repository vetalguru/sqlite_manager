#ifndef SQLITE_MANAGER_GUI_WTL_RUN_SQL_DIALOG_H
#define SQLITE_MANAGER_GUI_WTL_RUN_SQL_DIALOG_H

#include <string>

#include "gui/wtl/resource.h"
#include "gui/wtl/wtl.h"

namespace sqlite_manager_gui::wtl {

// A modal dialog with a multi-line editor for an ad-hoc SQL statement. The
// caller reads sql after DoModal() returns IDOK.
class CRunSqlDialog : public CDialogImpl<CRunSqlDialog> {
public:
    enum { IDD = IDD_RUN_SQL };

    std::wstring sql;

    // clang-format off
    BEGIN_MSG_MAP(CRunSqlDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()
    // clang-format on

    LRESULT OnOK(WORD /*code*/, WORD /*id*/, HWND /*ctl*/, BOOL& /*handled*/) {
        CWindow edit = GetDlgItem(IDC_SQL_TEXT);
        const int length = edit.GetWindowTextLength();
        std::wstring text(static_cast<size_t>(length) + 1, L'\0');
        const int copied = edit.GetWindowText(text.data(), length + 1);
        text.resize(static_cast<size_t>(copied));
        sql = std::move(text);
        EndDialog(IDOK);
        return 0;
    }

    LRESULT OnCancel(WORD /*code*/, WORD /*id*/, HWND /*ctl*/,
                     BOOL& /*handled*/) {
        EndDialog(IDCANCEL);
        return 0;
    }
};

}  // namespace sqlite_manager_gui::wtl

#endif  // SQLITE_MANAGER_GUI_WTL_RUN_SQL_DIALOG_H
