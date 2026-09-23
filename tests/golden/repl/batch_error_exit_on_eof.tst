=== name
an error in piped input sets exit 1 even when input ends without .quit
=== args
--batch
=== stdin
SELECT * FROM nope;
SELECT 2 AS y;
=== exit
1
=== out
+---+
| y |
+---+
| 2 |
+---+
=== err-contains
no such table: nope
