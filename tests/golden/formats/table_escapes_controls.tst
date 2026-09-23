=== name
control characters in a value are escaped so the frame stays intact
=== sql
SELECT 'a' || char(10) || 'b' || char(9) || 'c' || char(27) AS t
=== out
+-------------+
| t           |
+-------------+
| a\nb\tc\x1b |
+-------------+
