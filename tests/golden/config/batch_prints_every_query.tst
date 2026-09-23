=== name
each query in a batch prints its own result
=== sql
SELECT 1 AS a; SELECT 'x' AS b;
=== out
+---+
| a |
+---+
| 1 |
+---+
+---+
| b |
+---+
| x |
+---+
