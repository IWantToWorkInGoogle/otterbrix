-- @database pax_mixed_checkpoint
CREATE TABLE rows (
    id integer,
    category string,
    payload string,
    amount integer,
    flag integer,
    score double,
    region_id integer,
    filler01 integer,
    filler02 integer,
    filler03 integer,
    filler04 integer,
    filler05 integer,
    filler06 integer,
    filler07 integer,
    filler08 integer
) WITH (storage = 'disk');

-- @load_csv ../data/mixed_schema.tbl rows |
