RUNNER=build-release/benchmark/runner/benchmark_runner
BENCH=/home/usrname/otterbrix/benchmark
OUT=benchmark/results/manual-run

"$RUNNER" --disk --layout=columnar --load-only \
  --group='^tpch$' \
  --benchmarks="$BENCH"

"$RUNNER" --disk --layout=columnar --skip-load \
  --group='^tpch$' \
  --benchmarks="$BENCH" \
  --runs=3 \
  --out="$OUT/tpch_auto.csv" \
  'tpch/q'