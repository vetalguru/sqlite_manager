# Example databases

`demo.sql` builds a demo database with a broad schema — ten tables
covering every SQLite storage class and many declared column types
(INTEGER, TEXT, REAL, NUMERIC/DECIMAL, BLOB, BOOLEAN, DATE/DATETIME,
untyped, …), two views, several indexes, a trigger, and sample rows
(including NULLs and blobs).

Build it with the CLI (or `sqlite3`), from the repository root:

```bash
sqlite-manager --batch examples/demo.db < examples/demo.sql
# or: sqlite3 examples/demo.db < examples/demo.sql
```

Then open `examples/demo.db` in the GUI, or query it from the CLI:

```bash
sqlite-manager examples/demo.db "SELECT * FROM order_summary"
```

The generated `demo.db` is a build artifact and is not tracked in git.
