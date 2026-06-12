SELECT COUNT(*) AS row_count FROM pax_string.rows;
SELECT COUNT(*) AS north_count FROM pax_string.rows WHERE region = 'north';
SELECT city, COUNT(*) AS city_count FROM pax_string.rows WHERE region = 'north' GROUP BY city ORDER BY city;
SELECT name, note FROM pax_string.rows WHERE visits = 54321;
SELECT COUNT(*) AS visit_count, SUM(amount) AS amount_sum FROM pax_string.rows WHERE visits >= 20000 AND visits < 40000;
SELECT note FROM pax_string.rows WHERE city = 'city_005' AND visits >= 10000 AND visits < 12000 ORDER BY note;
SELECT name FROM pax_string.rows WHERE visits = 18000;
SELECT region, COUNT(*) AS tag_count FROM pax_string.rows WHERE tag = 'tag_03' GROUP BY region ORDER BY region;
