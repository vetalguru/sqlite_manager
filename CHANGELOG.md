# Changelog

All notable changes to this project are documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

The library and the CLI are versioned independently
(`MAJOR.MINOR.PATCH`, see [Versioning](README.md#versioning)) and released
under separate tags (`lib-v*`, `cli-v*`). Entries below note which component
they affect; the release procedure is in [RELEASING.md](RELEASING.md).

No release has been tagged yet, so everything so far is under _Unreleased_.

## [Unreleased]

### Added

- **Library** — RAII wrappers `Connection`, `Statement`, and `Transaction`
  over the SQLite C API; move-only, with automatic `close`/`finalize`.
- **Library** — a typed `Result<T>` / `Status` error model mapping SQLite
  result codes to an `ErrorCode` enum, checked against `sqlite3.h` at compile
  time.
- **Library** — `Connection::LastInsertRowId()`, `Connection::Changes()`, and
  `Connection::BusyTimeout()`.
- **Library** — bind statement parameters by name (`:name`) in addition to by
  position, and `Statement::ColumnType()` reporting the SQLite column type.
- **CLI** — `sqlite-manager` shell: run a single statement or an interactive
  REPL with multi-line SQL.
- **CLI** — query output formats via `--format table|csv|json`; JSON preserves
  SQLite types (numbers unquoted, `NULL` as `null`).
- **CLI** — dot commands `.help`, `.tables`, `.schema [TABLE]`, and
  `.read FILE`.
- **CLI** — interactive command history persisted to
  `~/.sqlite_manager_history` across sessions.
- **CLI** — `.deb` packaging with a man page, and a CMake `uninstall` target.
- **Tooling** — CI (debug/release), an ASan/UBSan sanitizers pipeline, a
  clang-tidy lint pipeline with a matching `lint` target, and a `.clang-format`
  config applied across the codebase.

[Unreleased]: https://github.com/vetalguru/sqlite_manager/commits/master
