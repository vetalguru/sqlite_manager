=== name
the .tables dot-command lists tables sorted
=== args
--batch
=== stdin
CREATE TABLE bbb (x);
CREATE TABLE aaa (x);
.tables
.quit
=== out
OK
OK
+------+
| name |
+------+
| aaa  |
| bbb  |
+------+
