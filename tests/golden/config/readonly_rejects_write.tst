=== name
a readonly database rejects writes
=== db
file-ro
=== setup
CREATE TABLE t (x);
=== sql
INSERT INTO t VALUES (1)
=== exit
1
=== out
=== err-contains
readonly
