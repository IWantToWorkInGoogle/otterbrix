SELECT COUNT(*) AS row_count, SUM(amount) AS total_amount FROM pax_mixed.rows;
SELECT SUM(amount) AS flag_amount FROM pax_mixed.rows WHERE flag = 1;
SELECT region_id, SUM(amount) AS amount FROM pax_mixed.rows WHERE category = 'C' GROUP BY region_id ORDER BY region_id;
SELECT payload FROM pax_mixed.rows WHERE amount = 158005 ORDER BY payload;
SELECT SUM(amount) AS amount_range, AVG(score) AS avg_score FROM pax_mixed.rows WHERE amount >= 50000 AND amount < 120000;
SELECT COUNT(*) AS selective_count FROM pax_mixed.rows WHERE filler04 = 17 AND flag = 0;
SELECT category, payload, amount FROM pax_mixed.rows WHERE id = 65432;
SELECT category, SUM(amount) AS amount FROM pax_mixed.rows WHERE id >= 40000 AND id < 70000 GROUP BY category ORDER BY category;
