=== name
a bad statement reports but the shell keeps going
=== args
--batch
=== stdin
SELEKT 1;
SELECT 2;
.quit
=== out
+---+
| 2 |
+---+
| 2 |
+---+
=== err-contains
Error:
