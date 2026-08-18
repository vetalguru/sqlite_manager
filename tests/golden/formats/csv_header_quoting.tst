=== name
a column name with a comma is quoted in the csv header
=== args
--format
csv
=== sql
SELECT 1 AS "a,b"
=== out
"a,b"
1
