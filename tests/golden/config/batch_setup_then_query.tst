=== name
a batch may create a table and query it, printing only the rows
=== sql
CREATE TABLE t (x); INSERT INTO t VALUES (5); SELECT x FROM t;
=== out
+---+
| x |
+---+
| 5 |
+---+
