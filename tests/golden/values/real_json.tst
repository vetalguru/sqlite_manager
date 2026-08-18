=== name
reals render as unquoted json numbers
=== args
--format
json
=== sql
SELECT 1.5 AS r, -3.0 AS n
=== out
[
  {"r": 1.5, "n": -3.0}
]
