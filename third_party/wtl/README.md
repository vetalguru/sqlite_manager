# WTL (Windows Template Library)

Vendored headers for the opt-in Windows GUI (`gui/wtl`). WTL is a header-only
C++ library layered on ATL; only its `Include/` directory is required.

- **Version:** WTL 10.0.10320 (Release, 2020-11-16)
- **Source:** <https://wtl.sourceforge.io/> (`WTL10_10320_Release.zip`)
- **License:** Microsoft Public License (MS-PL) — see [MS-PL.txt](MS-PL.txt)

Only `Include/` and the license are vendored; the release's samples and
AppWizard are omitted. ATL, which WTL extends, is supplied by the MSVC
toolchain (the Visual Studio "Desktop development with C++" / ATL component)
and is not vendored here.

The `wtl` CMake INTERFACE target in this directory exposes `Include/`; it is
linked only by the Windows GUI executable.
