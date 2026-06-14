#!/usr/bin/env python3
"""Generate a deterministic, readable SSB-style dataset for local benchmarks."""

from __future__ import annotations

import argparse
import calendar
from pathlib import Path


REGIONS = ["AMERICA", "ASIA", "EUROPE"]
NATIONS = {
    "AMERICA": ["UNITED STATES", "CANADA", "BRAZIL"],
    "ASIA": ["CHINA", "INDIA", "JAPAN"],
    "EUROPE": ["UNITED KINGDOM", "GERMANY", "FRANCE"],
}
CITY_BY_NATION = {
    "UNITED STATES": ["UNITED ST1", "UNITED ST5", "UNITED KI1", "UNITED KI5"],
    "CANADA": ["CANADA1", "CANADA5"],
    "BRAZIL": ["BRAZIL1", "BRAZIL5"],
    "CHINA": ["CHINA1", "CHINA5"],
    "INDIA": ["INDIA1", "INDIA5"],
    "JAPAN": ["JAPAN1", "JAPAN5"],
    "UNITED KINGDOM": ["UNITED KI1", "UNITED KI5"],
    "GERMANY": ["GERMANY1", "GERMANY5"],
    "FRANCE": ["FRANCE1", "FRANCE5"],
}
MONTH_NAMES = {
    1: "Jan",
    2: "Feb",
    3: "Mar",
    4: "Apr",
    5: "May",
    6: "Jun",
    7: "Jul",
    8: "Aug",
    9: "Sep",
    10: "Oct",
    11: "Nov",
    12: "Dec",
}


def write_rows(path: Path, header: list[str], rows) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as out:
        out.write("|".join(header) + "\n")
        for row in rows:
            out.write("|".join(str(value) for value in row) + "\n")


def date_rows() -> list[list[object]]:
    rows: list[list[object]] = []
    for year in range(1992, 1999):
        daynum = 1
        for month in range(1, 13):
            day = 1
            datekey = year * 10000 + month * 100 + day
            week = 6 if month == 2 else min(53, (month - 1) * 4 + 1)
            rows.append(
                [
                    datekey,
                    f"{year}-{month:02d}-{day:02d}",
                    calendar.day_name[calendar.weekday(year, month, day)],
                    MONTH_NAMES[month],
                    year,
                    year * 100 + month,
                    f"{MONTH_NAMES[month]}{year}",
                    1,
                    day,
                    daynum,
                    month,
                    week,
                    "Winter" if month in (12, 1, 2) else "Regular",
                    0,
                    0,
                    0,
                    1,
                ]
            )
            daynum += 31
    return rows


def customer_rows(count: int) -> list[list[object]]:
    rows: list[list[object]] = []
    nations = [(region, nation) for region in REGIONS for nation in NATIONS[region]]
    for idx in range(1, count + 1):
        region, nation = nations[(idx - 1) % len(nations)]
        cities = CITY_BY_NATION[nation]
        city = cities[(idx - 1) % len(cities)]
        rows.append(
            [
                idx,
                f"Customer#{idx:06d}",
                f"{idx} Market Street",
                city,
                nation,
                region,
                f"{10 + idx % 90}-{100 + idx % 900}-{1000 + idx % 9000}",
                f"SEGMENT{idx % 5}",
            ]
        )
    return rows


def supplier_rows(count: int) -> list[list[object]]:
    rows: list[list[object]] = []
    nations = [(region, nation) for region in REGIONS for nation in NATIONS[region]]
    for idx in range(1, count + 1):
        region, nation = nations[(idx * 2 - 1) % len(nations)]
        if idx % 10 == 0:
            region, nation = "AMERICA", "UNITED STATES"
        cities = CITY_BY_NATION[nation]
        city = cities[(idx - 1) % len(cities)]
        rows.append(
            [
                idx,
                f"Supplier#{idx:06d}",
                f"{idx} Supply Road",
                city,
                nation,
                region,
                f"{20 + idx % 80}-{100 + idx % 900}-{1000 + idx % 9000}",
            ]
        )
    return rows


def part_rows(count: int) -> list[list[object]]:
    rows: list[list[object]] = []
    colors = ["almond", "blue", "green", "red", "yellow"]
    for idx in range(1, count + 1):
        mfgr_no = 1 + (idx % 5)
        mfgr = f"MFGR#{mfgr_no}"
        category_no = 11 + (idx % 5)
        if idx % 11 == 0:
            category_no = 12
        if idx % 17 == 0:
            category_no = 14
        brand_suffix = 2221 + (idx % 8) if idx % 7 == 0 else (mfgr_no * 1000 + category_no * 10 + idx % 10)
        rows.append(
            [
                idx,
                f"Part#{idx:06d}",
                mfgr,
                f"MFGR#{category_no}",
                f"MFGR#{brand_suffix}",
                colors[idx % len(colors)],
                f"TYPE{idx % 7}",
                1 + idx % 50,
                f"CONT{idx % 4}",
            ]
        )
    return rows


def lineorder_rows(count: int, customers: int, parts: int, suppliers: int) -> list[list[object]]:
    rows: list[list[object]] = []
    dates = [row[0] for row in date_rows()]
    special_dates = [19930101, 19940101, 19940201, 19971201, 19980101]
    priorities = ["1-URGENT", "2-HIGH", "3-MEDIUM", "4-NOT SPECIFIED"]
    modes = ["AIR", "RAIL", "TRUCK", "SHIP"]
    for idx in range(1, count + 1):
        cust = 1 + (idx % customers)
        part = 1 + ((idx * 7) % parts)
        supp = 1 + ((idx * 13) % suppliers)
        if idx % 97 == 0:
            cust = 1
            supp = 10
            part = 17
        orderdate = special_dates[idx % len(special_dates)] if idx % 5 == 0 else dates[idx % len(dates)]
        quantity = 1 + (idx % 50)
        discount = 1 + (idx % 10)
        extended = 1000 + (idx % 10000)
        total = extended * quantity
        revenue = total - (total * discount // 100)
        supply = revenue // 2
        rows.append(
            [
                idx,
                1 + (idx % 7),
                cust,
                part,
                supp,
                orderdate,
                priorities[idx % len(priorities)],
                idx % 3,
                quantity,
                extended,
                total,
                discount,
                revenue,
                supply,
                idx % 9,
                orderdate,
                modes[idx % len(modes)],
            ]
        )
    return rows


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default=str(Path(__file__).resolve().parents[1] / "data" / "ssb"))
    parser.add_argument("--lineorder-rows", type=int, default=50000)
    parser.add_argument("--customers", type=int, default=3000)
    parser.add_argument("--parts", type=int, default=2000)
    parser.add_argument("--suppliers", type=int, default=300)
    args = parser.parse_args()

    out = Path(args.out)
    write_rows(
        out / "date.tbl",
        [
            "d_datekey",
            "d_date",
            "d_dayofweek",
            "d_month",
            "d_year",
            "d_yearmonthnum",
            "d_yearmonth",
            "d_daynuminweek",
            "d_daynuminmonth",
            "d_daynuminyear",
            "d_monthnuminyear",
            "d_weeknuminyear",
            "d_sellingseason",
            "d_lastdayinweekfl",
            "d_lastdayinmonthfl",
            "d_holidayfl",
            "d_weekdayfl",
        ],
        date_rows(),
    )
    write_rows(
        out / "customer.tbl",
        ["c_custkey", "c_name", "c_address", "c_city", "c_nation", "c_region", "c_phone", "c_mktsegment"],
        customer_rows(args.customers),
    )
    write_rows(
        out / "supplier.tbl",
        ["s_suppkey", "s_name", "s_address", "s_city", "s_nation", "s_region", "s_phone"],
        supplier_rows(args.suppliers),
    )
    write_rows(
        out / "part.tbl",
        ["p_partkey", "p_name", "p_mfgr", "p_category", "p_brand1", "p_color", "p_type", "p_size", "p_container"],
        part_rows(args.parts),
    )
    write_rows(
        out / "lineorder.tbl",
        [
            "lo_orderkey",
            "lo_linenumber",
            "lo_custkey",
            "lo_partkey",
            "lo_suppkey",
            "lo_orderdate",
            "lo_orderpriority",
            "lo_shippriority",
            "lo_quantity",
            "lo_extendedprice",
            "lo_ordtotalprice",
            "lo_discount",
            "lo_revenue",
            "lo_supplycost",
            "lo_tax",
            "lo_commitdate",
            "lo_shipmode",
        ],
        lineorder_rows(args.lineorder_rows, args.customers, args.parts, args.suppliers),
    )


if __name__ == "__main__":
    main()
