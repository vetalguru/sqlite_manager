=== name
a write in the REPL against a readonly database is rejected
=== args
--batch
--readonly
=== stdin
CREATE TABLE t (x);
.quit
=== out
=== err-contains
attempt to write a readonly database
