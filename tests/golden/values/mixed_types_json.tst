=== name
a row mixing integer, real, text and NULL renders faithfully in json
=== args
--format
json
=== sql
SELECT 1 AS i, 1.5 AS r, 'x' AS t, NULL AS n
=== out
[
  {"i": 1, "r": 1.5, "t": "x", "n": null}
]
