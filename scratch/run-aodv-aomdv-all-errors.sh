#!/usr/bin/env bash
set -euo pipefail

CSV="aodv-aomdv-results.csv"
OUTDIR="comparison-graphs-aodv-aomdv"

rm -f "$CSV"

for err in 0 0.1 0.2; do
  ./ns3 run "scratch/08-extended-sim.cc --protocol=AODV --errorRate=${err} --csv=${CSV}"
  ./ns3 run "scratch/08-extended-sim.cc --protocol=AOMDV --errorRate=${err} --csv=${CSV}"
done

python3 scratch/plot-aodv-aomdv-comparison.py \
  --csv "$CSV" \
  --outdir "$OUTDIR"
