=== name
a newline inside a field forces csv quoting across lines
=== args
--format
csv
=== sql
SELECT 'a' || char(10) || 'b' AS c
=== out
c
"a
b"
