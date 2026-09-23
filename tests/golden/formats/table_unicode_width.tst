=== name
the table sizes utf-8 text by characters, not bytes
=== sql
SELECT 'Привет' AS name UNION ALL SELECT 'ab'
=== out
+--------+
| name   |
+--------+
| Привет |
| ab     |
+--------+
