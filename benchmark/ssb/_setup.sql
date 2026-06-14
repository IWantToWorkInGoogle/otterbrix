-- @database ssb
CREATE TABLE customer (
    c_custkey integer,
    c_name string,
    c_address string,
    c_city string,
    c_nation string,
    c_region string,
    c_phone string,
    c_mktsegment string
) WITH (storage = 'disk');

CREATE TABLE dim_date (
    d_datekey integer,
    d_date string,
    d_dayofweek string,
    d_month string,
    d_year integer,
    d_yearmonthnum integer,
    d_yearmonth string,
    d_daynuminweek integer,
    d_daynuminmonth integer,
    d_daynuminyear integer,
    d_monthnuminyear integer,
    d_weeknuminyear integer,
    d_sellingseason string,
    d_lastdayinweekfl integer,
    d_lastdayinmonthfl integer,
    d_holidayfl integer,
    d_weekdayfl integer
) WITH (storage = 'disk');

CREATE TABLE lineorder (
    lo_orderkey integer,
    lo_linenumber integer,
    lo_custkey integer,
    lo_partkey integer,
    lo_suppkey integer,
    lo_orderdate integer,
    lo_orderpriority string,
    lo_shippriority integer,
    lo_quantity integer,
    lo_extendedprice integer,
    lo_ordtotalprice integer,
    lo_discount integer,
    lo_revenue integer,
    lo_supplycost integer,
    lo_tax integer,
    lo_commitdate integer,
    lo_shipmode string
) WITH (storage = 'disk');

CREATE TABLE part (
    p_partkey integer,
    p_name string,
    p_mfgr string,
    p_category string,
    p_brand1 string,
    p_color string,
    p_type string,
    p_size integer,
    p_container string
) WITH (storage = 'disk');

CREATE TABLE supplier (
    s_suppkey integer,
    s_name string,
    s_address string,
    s_city string,
    s_nation string,
    s_region string,
    s_phone string
) WITH (storage = 'disk');

-- @load_csv ../data/ssb/customer.tbl customer |
-- @load_csv ../data/ssb/date.tbl dim_date |
-- @load_csv ../data/ssb/part.tbl part |
-- @load_csv ../data/ssb/supplier.tbl supplier |
-- @load_csv ../data/ssb/lineorder.tbl lineorder |
