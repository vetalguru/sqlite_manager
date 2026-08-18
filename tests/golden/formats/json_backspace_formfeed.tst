=== name
json escapes backspace and form feed
=== args
--format
json
=== sql
SELECT char(8) || char(12) AS t
=== out
[
  {"t": "\b\f"}
]
