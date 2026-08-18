=== name
data seeded into a file database is queryable
=== db
file
=== setup
CREATE TABLE t (x);
INSERT INTO t VALUES (42);
=== sql
SELECT x FROM t
=== out
+----+
| x  |
+----+
| 42 |
+----+
