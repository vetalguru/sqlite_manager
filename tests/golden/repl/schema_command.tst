=== name
the .schema dot-command prints the stored CREATE statement
=== args
--batch
=== stdin
CREATE TABLE t (id INTEGER, name TEXT);
.schema
.quit
=== out
OK
CREATE TABLE t (id INTEGER, name TEXT);
