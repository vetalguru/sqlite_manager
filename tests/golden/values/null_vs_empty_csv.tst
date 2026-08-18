=== name
csv renders empty string and NULL both as empty fields
=== args
--format
csv
=== sql
SELECT '' AS empty, NULL AS nul
=== out
empty,nul
,
