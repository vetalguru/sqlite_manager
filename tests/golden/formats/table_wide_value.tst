=== name
a long value widens its whole column
=== sql
SELECT 'x' AS a UNION ALL SELECT 'wide value'
=== out
+------------+
| a          |
+------------+
| x          |
| wide value |
+------------+
