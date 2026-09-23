=== name
.schema on an unknown table reports to stderr
=== args
--batch
=== stdin
.schema nope
.quit
=== exit
1
=== out
=== err-contains
No such table: nope
