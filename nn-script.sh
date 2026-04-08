#!/bin/bash

# # NODES
# MODE="wired"
# CURRENT_PROGRESS=0
# TOTAL_PROGRESS=15

# echo "Starting simulations with varying number of nodes..."
# for n in 20 40 60 80 100; do
#   echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS)"
#   # flow is half of the number of nodes
#   flow=$((n/2))
#   ./ns3 run "scratch/manet-unified-sim.cc \
#     --mode=$MODE \
#     --nodes=$n \
#     --flows=$flow \
#     --pps=30 \
#     --speed=15 \
#     --time=20 \
#     --seed=1 \
#     --run=1 \
#     --csv=results/nn-test-wired-nodes.csv"
# done

# # Flows
# echo "Starting simulations with varying FLOW..."
# for f in 10 20 30 40 50; do
#   echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS)"
#   ./ns3 run "scratch/manet-unified-sim.cc \
#     --mode=$MODE \
#     --nodes=20 \
#     --flows=$f \
#     --pps=30 \
#     --speed=15 \
#     --time=20 \
#     --seed=1 \
#     --run=1 \
#     --csv=results/nn-test-wired-flows.csv"
# done


# # PPS
# echo "Starting simulations with varying PPS..."
# for p in 10 20 30 40 50; do
#   echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS)"
#   ./ns3 run "scratch/manet-unified-sim.cc \
#     --mode=$MODE \
#     --nodes=20 \
#     --flows=20 \
#     --pps=$p \
#     --speed=15 \
#     --time=20 \
#     --seed=1 \
#     --run=1 \
#     --csv=results/nn-test-wired-pps.csv"
# done

#Speed
echo "Starting simulations with varying PPS..."
for sp in 5 10 15 20 25; do
  echo "SIM ($((++CURRENT_PROGRESS))/$TOTAL_PROGRESS)"
  ./ns3 run "scratch/manet-unified-sim.cc \
    --mode=$MODE \
    --nodes=20 \
    --flows=20 \
    --pps=10 \
    --speed=$sp \
    --time=20 \
    --seed=1 \
    --run=1 \
    --csv=results/wireless-csvs/nn-test-speed.csv"
done





