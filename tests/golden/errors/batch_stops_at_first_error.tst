=== name
a batch stops at its first failing statement
=== sql
SELECT 1 AS a; SELECT * FROM missing; SELECT 2 AS b;
=== exit
1
=== out
+---+
| a |
+---+
| 1 |
+---+
=== err-contains
no such table: missing
