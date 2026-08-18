=== name
table columns pad to the widest value
=== sql
SELECT 1 AS a, 'x' AS b UNION ALL SELECT 22, 'y'
=== out
+----+---+
| a  | b |
+----+---+
| 1  | x |
| 22 | y |
+----+---+
