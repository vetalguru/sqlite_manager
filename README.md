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
`.schema [TABLE]`, `.read FILE`, `.quit` / `.exit` (or Ctrl-D). Command
history is saved to `~/.sqlite_manager_history` and restored next run.

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

## Usage

The examples below call `sqlite-manager` for brevity; if you have not installed
the `.deb`, use the built binary at `./build/debug/cli/sqlite-manager`.

### Create and query a database

```bash
sqlite-manager app.db "CREATE TABLE ammo (id INTEGER, name TEXT, grains REAL)"
sqlite-manager app.db "INSERT INTO ammo VALUES (1,'M855',62.0),(2,'SMK 175',175.0)"
sqlite-manager app.db "SELECT * FROM ammo"
```

```
+----+---------+--------+
| id | name    | grains |
+----+---------+--------+
| 1  | M855    | 62.0   |
| 2  | SMK 175 | 175.0  |
+----+---------+--------+
```

### Output formats

```bash
sqlite-manager --format csv app.db "SELECT id, name FROM ammo"
```

```
id,name
1,M855
2,SMK 175
```

```bash
sqlite-manager --format json app.db "SELECT id, name FROM ammo"
```

```json
[
  {"id": 1, "name": "M855"},
  {"id": 2, "name": "SMK 175"}
]
```

Values keep their JSON types — numbers stay unquoted (`1`, not `"1"`) and SQL
NULL becomes `null`. It pipes cleanly into jq:

```bash
sqlite-manager --format json app.db "SELECT id, name FROM ammo" | jq '.[].name'
```

```
"M855"
"SMK 175"
```

### Interactive shell

```
$ sqlite-manager app.db
sql> .tables
+------+
| name |
+------+
| ammo |
+------+
sql> SELECT count(*) AS n FROM ammo;
+---+
| n |
+---+
| 2 |
+---+
sql> .quit
```

### Batch mode: pipe a script

```bash
sqlite-manager --batch app.db < schema.sql

sqlite-manager --batch app.db <<'SQL'
CREATE TABLE t (x);
INSERT INTO t VALUES (1), (2), (3);
SELECT sum(x) AS total FROM t;
SQL
```

### Scratch and read-only

```bash
# in-memory database, nothing written to disk
sqlite-manager :memory: "SELECT 6 * 7 AS answer"

# open read-only; writes are rejected
sqlite-manager --readonly app.db "SELECT * FROM ammo"
```

## Library

Link against the `sqlite_manager` target and use the RAII wrappers directly:

```cpp
#include "sqlite_manager/connection.h"

sqlite_manager::Connection db;
if (!db.Open("app.db").ok()) { /* inspect the returned Status::error() */ }

db.BusyTimeout(1000);   // wait up to 1s if the database is locked

db.Execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
db.Execute("INSERT INTO users (name) VALUES ('ada')");

// Rowid of the row just inserted.
const std::int64_t id = db.LastInsertRowId();

db.Execute("UPDATE users SET name = 'Ada' WHERE id = 1");
const std::int64_t updated = db.Changes();   // rows affected: 1
```

Prepared statements bind parameters by position (`?`) or by name (`:name`):

```cpp
#include "sqlite_manager/statement.h"

auto q = sqlite_manager::Statement::Prepare(
    db, "SELECT name FROM users WHERE id = :id");
if (q) {
    q.value().BindInt64(":id", id);
    while (q.value().Step().value() ==
           sqlite_manager::Statement::StepResult::kRow) {
        const std::string name = q.value().ColumnText(0);
    }
}
```

## Tests

```bash
ctest --preset linux-debug            # or: ctest --test-dir build/debug
```

## Checks

**Static analysis (clang-tidy)** — the `lint` target runs exactly what the
Lint CI workflow does (findings are errors); run it before pushing:

```bash
cmake --preset linux-debug              # once, to generate the compile database
cmake --build build/debug --target lint
```

CI pins `clang-tidy-18`; install the same version locally
(`sudo apt-get install clang-tidy-18`) for results identical to CI. You can
also enable clang-tidy inline during a normal build:

```bash
cmake --preset linux-debug -DSQLITE_MANAGER_ENABLE_CLANG_TIDY=ON
cmake --build build/debug
```

**Sanitizers (AddressSanitizer + UndefinedBehaviorSanitizer)** — build and test
with the `linux-asan` preset:

```bash
cmake --preset linux-asan
cmake --build build/asan
ctest --preset linux-asan
```

**Coverage** — build instrumented, run the tests, and generate a
line-coverage report for project code (a terminal summary, HTML under
`build/coverage/coverage/`, and Cobertura XML):

```bash
cmake --preset linux-coverage
cmake --build build/coverage --target coverage
```

Requires `gcovr` (`sudo apt-get install gcovr`). CI runs it on every push
and uploads the report as an artifact.

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

Notable changes are recorded in [CHANGELOG.md](CHANGELOG.md); the release
procedure is in [RELEASING.md](RELEASING.md).

## License

See [LICENSE](LICENSE).
