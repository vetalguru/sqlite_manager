#ifndef SQLITE_MANAGER_GUI_WTL_WTL_H
#define SQLITE_MANAGER_GUI_WTL_WTL_H

// Shared ATL/WTL include header. The headers must appear in this exact
// order - atlbase before atlapp, and the application module declared between
// atlapp and the window headers - so clang-format must not sort them. The
// single definition of _Module lives in main.cpp; every other translation
// unit sees the extern declaration from here.
// clang-format off
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlframe.h>
#include <atlsplit.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atldlgs.h>
#include <atlcrack.h>
#include <atlmisc.h>
#include <commctrl.h>
// clang-format on

#endif  // SQLITE_MANAGER_GUI_WTL_WTL_H
