#!/bin/bash

set -euo pipefail

MODE="wireless"
CURRENT_PROGRESS=0
TOTAL_PROGRESS=20

OUT_DIR="results/wireless-csvs"
NODES_CSV="$OUT_DIR/nn-test-nodes.csv"
FLOWS_CSV="$OUT_DIR/nn-test-flows.csv"
PPS_CSV="$OUT_DIR/nn-test-pps.csv"
SPEED_CSV="$OUT_DIR/nn-test-speed.csv"

mkdir -p "$OUT_DIR"
rm -f "$NODES_CSV" "$FLOWS_CSV" "$PPS_CSV" "$SPEED_CSV"

# Teacher-provided sweep values are used directly here; internal scaling happens in code.
BASE_NODES=20
BASE_FLOWS=20
BASE_PPS=100
BASE_SPEED=15
BASE_TIME=20
BASE_TRAFFIC_START=2

echo "Starting simulations with varying number of nodes..."
for n in 20 40 60 80 100; do
  echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS)"
  ./ns3 run "scratch/manet-unified-sim.cc \
    --mode=$MODE \
    --nodes=$n \
    --flows=$BASE_FLOWS \
    --pps=$BASE_PPS \
    --speed=$BASE_SPEED \
    --time=$BASE_TIME \
    --trafficStart=$BASE_TRAFFIC_START \
    --seed=1 \
    --run=1 \
    --csv=$NODES_CSV"
done

# Flows
echo "Starting simulations with varying FLOW..."
for f in 10 20 30 40 50; do
  echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS)"
  ./ns3 run "scratch/manet-unified-sim.cc \
    --mode=$MODE \
    --nodes=$BASE_NODES \
    --flows=$f \
    --pps=$BASE_PPS \
    --speed=$BASE_SPEED \
    --time=$BASE_TIME \
    --trafficStart=$BASE_TRAFFIC_START \
    --seed=1 \
    --run=1 \
    --csv=$FLOWS_CSV"
done


# PPS
echo "Starting simulations with varying PPS..."
for p in 100 200 300 400 500; do
  echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS)"
  ./ns3 run "scratch/manet-unified-sim.cc \
    --mode=$MODE \
    --nodes=$BASE_NODES \
    --flows=$BASE_FLOWS \
    --pps=$p \
    --speed=$BASE_SPEED \
    --time=$BASE_TIME \
    --trafficStart=$BASE_TRAFFIC_START \
    --seed=1 \
    --run=1 \
    --csv=$PPS_CSV"
done

#Speed
echo "Starting simulations with varying speed..."
for sp in 5 10 15 20 25; do
  echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS)"
  ./ns3 run "scratch/manet-unified-sim.cc \
    --mode=$MODE \
    --nodes=$BASE_NODES \
    --flows=$BASE_FLOWS \
    --pps=$BASE_PPS \
    --speed=$sp \
    --time=$BASE_TIME \
    --trafficStart=$BASE_TRAFFIC_START \
    --seed=1 \
    --run=1 \
    --csv=$SPEED_CSV"
done





