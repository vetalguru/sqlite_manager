=== name
infinities have no json form and render as null
=== args
--format
json
=== sql
SELECT 1e999 AS pos, -1e999 AS neg, 1.5 AS finite
=== out
[
  {"pos": null, "neg": null, "finite": 1.5}
]
