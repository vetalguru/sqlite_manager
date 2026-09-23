=== name
a write in the REPL against a readonly database is rejected
=== args
--batch
--readonly
=== stdin
CREATE TABLE t (x);
.quit
=== exit
1
=== out
=== err-contains
attempt to write a readonly database
