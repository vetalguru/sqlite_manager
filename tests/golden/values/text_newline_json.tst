=== name
json escapes an embedded newline
=== args
--format
json
=== sql
SELECT 'a' || char(10) || 'b' AS t
=== out
[
  {"t": "a\nb"}
]
