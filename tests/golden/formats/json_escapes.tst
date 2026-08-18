=== name
json escapes quote, tab, carriage return and backslash
=== args
--format
json
=== sql
SELECT char(34) || char(9) || char(13) || char(92) AS t
=== out
[
  {"t": "\"\t\r\\"}
]
