=== name
a blob renders as its bytes, quoted in json
=== args
--format
json
=== sql
SELECT X'41' AS b
=== out
[
  {"b": "A"}
]
