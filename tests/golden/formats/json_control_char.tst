=== name
json emits a low control character as \u00xx
=== args
--format
json
=== sql
SELECT char(1) AS t
=== out
[
  {"t": "\u0001"}
]
