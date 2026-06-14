#!/usr/bin/env python3
"""Generate a deterministic synthetic ClickBench `hits` fixture sized for a
RAM-limited box. Matches the 104-column schema in _setup.sql. Distributions of
the queried columns (AdvEngineID, SearchPhrase, URL, Title, RegionID, UserID...)
are made selective so the standard ClickBench queries are meaningful."""
import argparse, random, re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SETUP = ROOT / "_setup.sql"
OUT = ROOT.parent / "data" / "clickbench" / "hits.csv"


def parse_columns():
    text = SETUP.read_text()
    body = text[text.index("(") + 1 : text.index(") WITH")]
    cols = []
    for line in body.splitlines():
        line = line.strip().rstrip(",")
        if not line or line.startswith("--"):
            continue
        parts = line.split()
        if len(parts) >= 2:
            cols.append((parts[0], parts[1].lower()))
    return cols


SEARCH = ["", "", "", "", "hotel kyiv", "cheap flight", "buy phone", "used car", "daily news"]
MODELS = ["", "", "", "iPhone", "Galaxy S", "Pixel", "Nokia"]


def value(name, typ, i, rnd):
    if name == "AdvEngineID":
        return 0 if rnd.random() < 0.9 else rnd.randint(1, 5)
    if name == "ResolutionWidth":
        return rnd.randint(1000, 2000)
    if name == "ResolutionHeight":
        return rnd.randint(700, 1200)
    if name == "UserID":
        return rnd.randint(1, 12000) * 36028797  # limited distinct, large magnitude
    if name == "RegionID":
        return rnd.randint(0, 100)
    if name == "CounterID":
        return rnd.randint(1, 200)
    if name == "SearchEngineID":
        return rnd.randint(0, 5)
    if name == "SearchPhrase":
        return rnd.choice(SEARCH)
    if name == "MobilePhoneModel":
        return rnd.choice(MODELS)
    if name == "MobilePhone":
        return rnd.randint(0, 10)
    if name == "URL":
        return "http://google.com/search?q=q" if rnd.random() < 0.05 else f"http://example.com/p{i % 1000}"
    if name == "Referer":
        return "http://google.com/" if rnd.random() < 0.04 else f"http://ref.example/{i % 500}"
    if name == "Title":
        return "Google Search Results" if rnd.random() < 0.03 else f"Page Title {i % 800}"
    if name == "EventTime":
        return f"2013-07-{1 + i % 28:02d} {i % 24:02d}:{i % 60:02d}:{(i * 7) % 60:02d}"
    if name in ("EventDate", "ClientEventTime", "LocalEventTime"):
        return f"2013-07-{1 + i % 28:02d}"
    if name == "WatchID":
        return rnd.randint(1, 10**18)
    # default by type
    if typ == "utinyint":
        return rnd.randint(0, 5)
    if typ in ("uinteger", "integer"):
        return rnd.randint(0, 1000)
    if typ in ("ubigint", "bigint"):
        return rnd.randint(0, 10**9)
    if typ == "string":
        return ""
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rows", type=int, default=60000)
    args = ap.parse_args()
    cols = parse_columns()
    assert len(cols) > 100, f"expected ~104 columns, got {len(cols)}"
    rnd = random.Random(42)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", encoding="utf-8") as f:
        f.write("|".join(n for n, _ in cols) + "\n")
        for i in range(1, args.rows + 1):
            f.write("|".join(str(value(n, t, i, rnd)) for n, t in cols) + "\n")
    print(f"wrote {args.rows} rows x {len(cols)} cols -> {OUT}")


if __name__ == "__main__":
    main()
