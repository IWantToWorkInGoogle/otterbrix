#!/usr/bin/env python3
"""Generate deterministic CSV fixtures for the PAX benchmark suite."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent
DATA = ROOT / "data"


def write_rows(path, header, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        f.write("|".join(header) + "\n")
        for row in rows:
            f.write("|".join(str(v) for v in row) + "\n")


def fixed_rows(n=150_000):
    header = [
        "id",
        "metric",
        "score",
        "flag",
        "region_id",
        "segment_id",
        "event_day",
        "discount",
        "quantity",
        "revenue",
        "cost",
    ] + [f"filler{i:02d}" for i in range(1, 21)]

    rows = []
    for i in range(1, n + 1):
        metric = (i * 37) % 10000
        quantity = 1 + (i % 50)
        discount = i % 11
        revenue = 1000 + ((i * 17) % 50000)
        cost = 300 + ((i * 13) % 20000)
        base = [
            i,
            metric,
            f"{((i * 19) % 100000) / 100.0:.2f}",
            i & 1,
            i % 12,
            i % 8,
            20240101 + (i % 120),
            discount,
            quantity,
            revenue,
            cost,
        ]
        fillers = [((i * (j + 3)) + j * 17) % 1000 for j in range(20)]
        rows.append(base + fillers)
    write_rows(DATA / "fixed_analytic.tbl", header, rows)


def string_rows(n=80_000):
    header = ["id", "name", "city", "region", "note", "tag", "visits", "amount"]
    regions = ["north", "south", "east", "west"]
    rows = []
    for i in range(1, n + 1):
        rows.append(
            [
                i,
                f"name_{i:06d}",
                f"city_{i % 64:03d}",
                regions[i % len(regions)],
                f"note_{i % 2048:04d}_bucket_{i % 17:02d}",
                f"tag_{i % 32:02d}",
                i,
                10 + ((i * 23) % 100000),
            ]
        )
    write_rows(DATA / "string_heavy.tbl", header, rows)


def mixed_rows(n=100_000):
    header = [
        "id",
        "category",
        "payload",
        "amount",
        "flag",
        "score",
        "region_id",
    ] + [f"filler{i:02d}" for i in range(1, 9)]
    cats = ["A", "B", "C", "D", "E", "F"]
    rows = []
    for i in range(1, n + 1):
        base = [
            i,
            cats[i % len(cats)],
            f"payload_{i % 4096:04d}_segment_{i % 23:02d}",
            (i * 29) % 200000,
            i & 1,
            f"{((i * 31) % 100000) / 200.0:.2f}",
            i % 16,
        ]
        fillers = [((i * (j + 5)) + j * 11) % 2048 for j in range(8)]
        rows.append(base + fillers)
    write_rows(DATA / "mixed_schema.tbl", header, rows)


def dml_fixed_rows(n=30_000):
    header = [
        "id",
        "group_id",
        "flag",
        "amount",
        "version",
        "filler01",
        "filler02",
        "filler03",
        "filler04",
        "filler05",
        "filler06",
    ]
    rows = []
    for i in range(1, n + 1):
        rows.append(
            [
                i,
                i % 128,
                0,
                100 + ((i * 31) % 100000),
                1,
                (i * 3) % 1000,
                (i * 5) % 1000,
                (i * 7) % 1000,
                (i * 11) % 1000,
                (i * 13) % 1000,
                (i * 17) % 1000,
            ]
        )
    write_rows(DATA / "dml_fixed.tbl", header, rows)


def dml_string_rows(n=20_000):
    header = ["id", "city", "status", "note", "amount"]
    cities = ["city_a", "city_b", "city_c", "city_d", "city_e"]
    rows = []
    for i in range(1, n + 1):
        rows.append(
            [
                i,
                cities[i % len(cities)],
                "cold",
                f"note_{i % 1024:04d}_bucket_{i % 19:02d}",
                100 + ((i * 41) % 50000),
            ]
        )
    write_rows(DATA / "dml_string.tbl", header, rows)


def main():
    fixed_rows()
    string_rows()
    mixed_rows()
    dml_fixed_rows()
    dml_string_rows()


if __name__ == "__main__":
    main()
