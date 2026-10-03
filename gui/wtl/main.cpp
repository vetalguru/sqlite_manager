// Entry point for the WTL GUI: it owns the single application module, pumps
// the message loop, and shows the main frame. WTL extends ATL, which ships
// with MSVC, so this builds only on Windows (SQLITE_MANAGER_BUILD_WTL_GUI).

// Opt into Common Controls v6 so the window uses the modern, themed controls.
#pragma comment(linker,                                                        \
                "\"/manifestdependency:type='win32' "                          \
                "name='Microsoft.Windows.Common-Controls' version='6.0.0.0' "  \
                "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' " \
                "language='*'\"")

#include "gui/wtl/wtl.h"

// The one and only application module (declared extern in wtl.h).
CAppModule _Module;

#include "gui/wtl/main_frame.h"

namespace {

int RunMessageLoop(int cmd_show) {
    CMessageLoop loop;
    _Module.AddMessageLoop(&loop);

    sqlite_manager_gui::wtl::CMainFrame frame;
    if (frame.CreateEx() == nullptr) return 0;
    frame.SetWindowText(L"SQLite Manager");
    frame.ShowWindow(cmd_show);
    frame.UpdateWindow();

    const int result = loop.Run();
    _Module.RemoveMessageLoop();
    return result;
}

}  // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmd_show) {
    const INITCOMMONCONTROLSEX icc{sizeof(INITCOMMONCONTROLSEX),
                                   ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES |
                                       ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES |
                                       ICC_STANDARD_CLASSES};
    ::InitCommonControlsEx(&icc);

    const HRESULT hr = _Module.Init(nullptr, instance);
    ATLASSERT(SUCCEEDED(hr));
    static_cast<void>(hr);

    const int result = RunMessageLoop(cmd_show);
    _Module.Term();
    return result;
}
