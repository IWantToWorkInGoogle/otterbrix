-- @database pax_string_checkpoint
CREATE TABLE rows (
    id integer,
    name string,
    city string,
    region string,
    note string,
    tag string,
    visits integer,
    amount integer
) WITH (storage = 'disk');

-- @load_csv ../data/string_heavy.tbl rows |
