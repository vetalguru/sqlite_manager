# Golden functional tests

Each `*.tst` file here is one end-to-end run of the `sqlite-manager` CLI:
a seeded database plus a command line and/or REPL input, together with the
exact output it must produce. The reference answer is **what the CLI
prints** — the code we own (argument parsing, execution, the table/CSV/JSON
writers) — not a raw SQL result set. `tests/golden_test.cpp` discovers
every file here and turns each into its own GoogleTest case.

## File format

Sections are introduced by a line `=== <key>` and run until the next
header. Lines before the first header are ignored (use them for comments).

| Key            | Meaning                                                        |
| -------------- | -------------------------------------------------------------- |
| `name`         | One-line human name (optional; defaults to the file path).     |
| `db`           | `memory` (default), `file`, `file-ro`, or `none`.              |
| `setup`        | SQL seeded into the database before the run (`file`/`file-ro`).|
| `file`         | Contents of an auxiliary file; `{FILE}` resolves to its path.  |
| `args`         | Extra CLI option tokens, **one per line**, before the db path. |
| `sql`          | Positional SQL argument — single-shot mode.                    |
| `stdin`        | Text fed on standard input — REPL mode (pair with `--batch`).  |
| `exit`         | Expected exit code (default `0`).                              |
| `out`          | Expected stdout, matched exactly.                              |
| `err`          | Expected stderr, matched exactly.                              |
| `err-contains` | Substring stderr must contain.                                 |

The command line is assembled as `[args] [db-path] [sql]`. `db: file-ro`
adds `--readonly` automatically and seeds through a read-write connection
first. `db: none` omits the path entirely (for missing-argument cases). A
`file` section writes a temp file and substitutes its path for `{FILE}` in
`args`, `sql` and `stdin` — used to drive `.read {FILE}`.

A body is its lines each joined with a trailing newline, matching the CLI,
whose output is always newline-terminated. If neither `err` nor
`err-contains` is present, stderr must be empty. A body line cannot itself
start with `=== `.

## Adding a case

Drop a new `*.tst` file in a subdirectory and rebuild — discovery re-runs
and the case appears as `Corpus/GoldenTest.MatchesReferenceOutput/<path>`.
To see one case: `ctest -R <name>` or run the test binary with
`--gtest_filter='*<path>*'`.
