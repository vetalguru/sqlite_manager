=== name
a REPL query ending in a comment still prints its rows
=== args
--batch
=== stdin
SELECT 7 AS n; -- lucky
=== out
+---+
| n |
+---+
| 7 |
+---+
