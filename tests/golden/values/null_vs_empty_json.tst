=== name
json distinguishes empty string from NULL
=== args
--format
json
=== sql
SELECT '' AS empty, NULL AS nul
=== out
[
  {"empty": "", "nul": null}
]
