=== name
64-bit integer boundaries survive as JSON numbers
=== args
--format
json
=== sql
SELECT -9223372036854775808 AS lo, 9223372036854775807 AS hi
=== out
[
  {"lo": -9223372036854775808, "hi": 9223372036854775807}
]
