=== name
csv doubles embedded quotes and wraps the field
=== args
--format
csv
=== sql
SELECT 'a"b' AS c
=== out
c
"a""b"
