=== name
the .read dot-command runs statements from a file
=== file
CREATE TABLE t (x);
INSERT INTO t VALUES (7);
SELECT x FROM t;
=== args
--batch
=== stdin
.read {FILE}
.quit
=== out
OK
OK
+---+
| x |
+---+
| 7 |
+---+
