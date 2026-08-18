=== name
a readonly database still serves reads
=== db
file-ro
=== setup
CREATE TABLE t (x);
INSERT INTO t VALUES (7);
=== sql
SELECT x FROM t
=== out
+---+
| x |
+---+
| 7 |
+---+
