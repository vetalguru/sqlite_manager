=== name
a quote in a column name is escaped in the json key
=== args
--format
json
=== sql
SELECT 1 AS "a""b"
=== out
[
  {"a\"b": 1}
]
