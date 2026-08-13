# sqlite_manager

[![CI](https://github.com/vetalguru/sqlite_manager/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/vetalguru/sqlite_manager/actions/workflows/ci.yml)
[![Sanitizers](https://github.com/vetalguru/sqlite_manager/actions/workflows/sanitizers.yml/badge.svg?branch=master)](https://github.com/vetalguru/sqlite_manager/actions/workflows/sanitizers.yml)

A lightweight C++17 wrapper over the SQLite C API, with a command-line shell.

## Features

- **RAII wrappers** — `Connection`, `Statement`, `Transaction` manage SQLite
  handles automatically; move-only, no manual `close`/`finalize`.
- **Typed errors** — a `Result<T>` / `Status` model maps SQLite result codes to
  an `ErrorCode` enum, kept in sync with `sqlite3.h` at compile time.
- **CLI shell** (`sqlite-manager`) — run one statement or an interactive REPL,
  with query results rendered as an aligned, framed table.
- Self-contained: SQLite and isocline are vendored under `third_party/`.

## Requirements

- CMake ≥ 3.22 and a build tool (Ninja in the presets below)
- A C++17 compiler (GCC or Clang)
- Linux, or Windows via WSL

GoogleTest is fetched automatically at configure time; SQLite and isocline are
bundled, so no system libraries are required.

## Build

```bash
# Debug
cmake --preset linux-debug
cmake --build build/debug

# Release
cmake --preset linux-release
cmake --build build/release
```

The binary is `build/<config>/cli/sqlite-manager`.

## Run

```bash
# Run one statement and exit
./build/debug/cli/sqlite-manager data.db "SELECT * FROM users"

# Interactive shell on an in-memory database
./build/debug/cli/sqlite-manager :memory:
```

In the shell, statements end with `;`. Dot commands: `.help`, `.tables`,
`.quit` / `.exit` (or Ctrl-D).

```
sql> SELECT 1 AS id, 'M855' AS name;
+----+------+
| id | name |
+----+------+
| 1  | M855 |
+----+------+
```

Options: `-h`/`--help`, `--version`, `--readonly` (open read-only),
`--batch` (suppress interactive prompts, for piping SQL from a file),
`--format table|csv|json` (query output format; `table` is the default).

```bash
# CSV or JSON output, e.g. to pipe into other tools
./build/debug/cli/sqlite-manager --format csv  data.db "SELECT * FROM users"
./build/debug/cli/sqlite-manager --format json data.db "SELECT * FROM users"
```

## Tests

```bash
ctest --preset linux-debug            # or: ctest --test-dir build/debug
```

## Checks

**Static analysis (clang-tidy)** — enable it as part of the build:

```bash
cmake --preset linux-debug -DSQLITE_MANAGER_ENABLE_CLANG_TIDY=ON
cmake --build build/debug
```

or run it standalone against the compile database:

```bash
clang-tidy -p build/debug src/*.cpp cli/*.cpp
```

**Sanitizers (AddressSanitizer + UndefinedBehaviorSanitizer)** — build and test
with the `linux-asan` preset:

```bash
cmake --preset linux-asan
cmake --build build/asan
ctest --preset linux-asan
```

## Packaging (.deb)

```bash
cd build/release && cpack -G DEB     # -> sqlite-manager_<version>_amd64.deb
```

Install and remove:

```bash
sudo apt install ./sqlite-manager_*_amd64.deb
sudo apt remove sqlite-manager
```

## Versioning

Each component versions independently: `MAJOR.MINOR` are set by hand in CMake,
while `PATCH` is derived from git — the number of commits touching the
component's sources since its last release tag (`lib-v*` for the library,
`cli-v*` for the CLI). See `cmake/GitVersion.cmake`.

## License

See [LICENSE](LICENSE).
