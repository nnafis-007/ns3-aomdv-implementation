#!/bin/bash

set -euo pipefail

MODE="wired"
CURRENT_PROGRESS=0
TOTAL_PROGRESS=15

# Teacher-provided sweep values are used directly here; internal scaling happens in code.
BASE_NODES=20
BASE_FLOWS=20
BASE_PPS=300
BASE_TIME=60
BASE_TRAFFIC_START=5

OUT_DIR="results/wired-csvs"
NODES_CSV="$OUT_DIR/nn-test-nodes.csv"
FLOWS_CSV="$OUT_DIR/nn-test-flows.csv"
PPS_CSV="$OUT_DIR/nn-test-pps.csv"

mkdir -p "$OUT_DIR"

# Keep this run reproducible by clearing old sweep outputs.
rm -f "$NODES_CSV" "$FLOWS_CSV" "$PPS_CSV"

echo "Starting WIRED simulations with varying number of nodes..."
for n in 20 40 60 80 100; do
  echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS) | Nodes=$n"
  ./ns3 run "scratch/manet-unified-sim.cc \
    --mode=$MODE \
    --nodes=$n \
    --flows=$BASE_FLOWS \
    --pps=$BASE_PPS \
    --time=$BASE_TIME \
    --trafficStart=$BASE_TRAFFIC_START \
    --seed=1 \
    --run=1 \
    --csv=$NODES_CSV"
done

echo "Starting WIRED simulations with varying number of flows..."
for f in 10 20 30 40 50; do
  echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS) | Flows=$f"
  ./ns3 run "scratch/manet-unified-sim.cc \
    --mode=$MODE \
    --nodes=$BASE_NODES \
    --flows=$f \
    --pps=$BASE_PPS \
    --time=$BASE_TIME \
    --trafficStart=$BASE_TRAFFIC_START \
    --seed=1 \
    --run=1 \
    --csv=$FLOWS_CSV"
done

echo "Starting WIRED simulations with varying PPS..."
for p in 100 200 300 400 500; do
  echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS) | PPS=$p"
  ./ns3 run "scratch/manet-unified-sim.cc \
    --mode=$MODE \
    --nodes=$BASE_NODES \
    --flows=$BASE_FLOWS \
    --pps=$p \
    --time=$BASE_TIME \
    --trafficStart=$BASE_TRAFFIC_START \
    --seed=1 \
    --run=1 \
    --csv=$PPS_CSV"
done

echo "WIRED sweep completed. Outputs are in: $OUT_DIR"