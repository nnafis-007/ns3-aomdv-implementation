rm -f baseline.csv error20.csv
./ns3 run "scratch/07-final-simulation.cc --protocol=AODV --errorRate=0   --csv=baseline.csv"
./ns3 run "scratch/07-final-simulation.cc --protocol=DSDV --errorRate=0   --csv=baseline.csv"
./ns3 run "scratch/07-final-simulation.cc --protocol=OLSR --errorRate=0   --csv=baseline.csv"
./ns3 run "scratch/07-final-simulation.cc --protocol=AODV --errorRate=0.2 --csv=error20.csv"
./ns3 run "scratch/07-final-simulation.cc --protocol=DSDV --errorRate=0.2 --csv=error20.csv"
./ns3 run "scratch/07-final-simulation.cc --protocol=OLSR --errorRate=0.2 --csv=error20.csv"

python3 scratch/plot-error-comparison.py \
    --csv1 baseline.csv --label1 "0% Error" \
    --csv2 error20.csv  --label2 "20% Error" \
    --outdir comparison-graphs-e20