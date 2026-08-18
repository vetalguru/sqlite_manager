=== name
csv quotes commas and empties NULL
=== args
--format
csv
=== sql
SELECT 'a,b' AS c, NULL AS d
=== out
c,d
"a,b",
