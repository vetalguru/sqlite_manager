=== name
a blob that is not valid utf-8 still yields valid json (hex)
=== args
--format
json
=== sql
SELECT X'FF00fe' AS b
=== out
[
  {"b": "ff00fe"}
]
