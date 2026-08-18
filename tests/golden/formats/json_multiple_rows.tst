=== name
several rows are comma-separated json objects
=== args
--format
json
=== sql
SELECT 1 AS n UNION ALL SELECT 2 UNION ALL SELECT 3
=== out
[
  {"n": 1},
  {"n": 2},
  {"n": 3}
]
