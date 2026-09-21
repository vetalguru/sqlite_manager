=== name
a query followed by a comment still prints its rows
=== sql
SELECT 1 AS a; -- note
=== out
+---+
| a |
+---+
| 1 |
+---+
