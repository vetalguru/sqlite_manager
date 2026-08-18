=== name
json emits numbers unquoted and NULL as null
=== args
--format
json
=== sql
SELECT 1 AS id, 'x' AS name UNION ALL SELECT 2, NULL
=== out
[
  {"id": 1, "name": "x"},
  {"id": 2, "name": null}
]
