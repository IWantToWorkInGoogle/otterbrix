SELECT SUM(revenue) AS total_revenue, SUM(cost) AS total_cost, COUNT(*) AS row_count FROM pax_fixed.rows;
SELECT SUM(revenue) AS january_revenue FROM pax_fixed.rows WHERE event_day >= 20240101 AND event_day < 20240132;
SELECT region_id, SUM(revenue) AS revenue FROM pax_fixed.rows WHERE segment_id = 3 GROUP BY region_id ORDER BY region_id;
SELECT SUM(revenue) AS metric_revenue FROM pax_fixed.rows WHERE metric = 777;
SELECT SUM(revenue) AS range_revenue, SUM(cost) AS range_cost FROM pax_fixed.rows WHERE metric >= 1000 AND metric < 4000;
SELECT SUM(revenue) AS selective_revenue FROM pax_fixed.rows WHERE filler07 = 13 AND discount BETWEEN 3 AND 7;
SELECT id, revenue, score FROM pax_fixed.rows WHERE id = 77777;
SELECT SUM(revenue) AS id_range_revenue FROM pax_fixed.rows WHERE id >= 60000 AND id < 80000;
