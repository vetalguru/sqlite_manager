=== name
.schema NAME prints only the matching table
=== args
--batch
=== stdin
CREATE TABLE a (x);
CREATE TABLE b (y);
.schema b
.quit
=== out
OK
OK
CREATE TABLE b (y);
