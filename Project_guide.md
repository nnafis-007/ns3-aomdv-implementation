# Project Guide: Running MANET Simulations Easily

This guide explains how to run the main simulation workflows in this project and how to run each simulation `.cc` file directly with arguments.

## 1) Quick Start

From project root:

```bash
cd /home/nn/buet-classes/322-network-lab/ns-3-dev
./ns3 build
```

For plotting scripts, ensure Python packages are available:

```bash
python3 -m pip install matplotlib pandas numpy
```

## 2) Minimal File Tree (Files Used by my simulation)

```text
ns-3-dev/
├── ns3
├── manet-unified-wired.sh
├── manet-unified-wireless.sh
├── scratch/
│   ├── run-all-protocols.sh
│   ├── run-aodv-aomdv-all-errors.sh
│   ├── 07-final-simulation.cc
│   ├── 08-extended-sim.cc
│   ├── manet-unified-sim.cc
│   ├── plot-error-comparison.py
│   └── plot-aodv-aomdv-comparison.py
├── src/
│   └── aomdv/
│       ├── CMakeLists.txt
│       ├── helper/
│       │   ├── aomdv-helper.h
│       │   └── aomdv-helper.cc
│       └── model/
│           ├── aomdv-routing-protocol.h
│           ├── aomdv-routing-protocol.cc
│           ├── aomdv-rtable.h
│           ├── aomdv-rtable.cc
│           ├── aomdv-packet.h
│           └── aomdv-packet.cc
└── results/
    ├── wired-csvs/
    └── wireless-csvs/
```

Here :
- `run-all-protocols.sh` runs `scratch/07-final-simulation.cc` and then plots with `scratch/plot-error-comparison.py`.
- `run-aodv-aomdv-all-errors.sh` runs `scratch/08-extended-sim.cc` and then plots with `scratch/plot-aodv-aomdv-comparison.py`.
- `manet-unified-wired.sh` and `manet-unified-wireless.sh` run `scratch/manet-unified-sim.cc` sweeps.
- `src/aomdv/*` is required by simulations that use protocol `AOMDV`.

## 3) Main Scripts

## 3.1 `scratch/run-all-protocols.sh`
- Clears: `baseline.csv`, `error10.csv`, `error20.csv`.
- Runs `07-final-simulation.cc` for protocols: `AODV`, `DSDV`, `OLSR`.
- Runs each protocol at error rates: `0`, `0.1`, `0.2`.
- Generates comparison plot using `plot-error-comparison.py` into `comparison-graphs-e20/`.

Run:

```bash
bash scratch/run-all-protocols.sh
```

## 3.2 `scratch/run-aodv-aomdv-all-errors.sh`
- Clears: `aodv-aomdv-results.csv`.
- For each error rate (`0`, `0.1`, `0.2`), runs `08-extended-sim.cc` for `AODV` then `AOMDV`.
- Generates output figure in `comparison-graphs-aodv-aomdv/`.

Run:

```bash
bash scratch/run-aodv-aomdv-all-errors.sh
```

## 3.3 `manet-unified-wired.sh`
- Uses mode `wired`.
- Runs 3 sweeps (nodes, flows, pps), each with 5 points (total 15 runs).
- Uses tuned baseline values and writes to:
  - `results/wired-csvs/nn-test-nodes.csv`
  - `results/wired-csvs/nn-test-flows.csv`
  - `results/wired-csvs/nn-test-pps.csv`

Run:

```bash
bash manet-unified-wired.sh
```

## 3.4 `manet-unified-wireless.sh`
- Uses mode `wireless`.
- Current active block sweeps **speed** only (`5 10 15 20 25`).
- Writes to `results/wireless-csvs/nn-test-speed.csv`.
- Nodes/flows/pps sweeps exist but are commented out in current script.

Run:

```bash
bash manet-unified-wireless.sh
```

## 4) Standalone Simulation Commands

for any scratch simulation:

```bash
./ns3 run "scratch/<simulation>.cc --arg1=... --arg2=..."
```

## 4.1 `scratch/07-final-simulation.cc`

### Supported arguments
- `--nodes` : number of MANET nodes
- `--time` : simulation time (seconds)
- `--protocol` : `AODV`, `DSDV`, `OLSR`
- `--errorRate` : packet error rate in `[0.0, 1.0]`
- `--minSpeed` : min node speed (m/s)
- `--speed` : max node speed (m/s)
- `--pause` : pause time (s)
- `--flows` : number of UDP flows
- `--packetSize` : UDP payload size (bytes)
- `--interval` : per-flow packet interval (seconds)
- `--lossPenaltyMs` : delay penalty for lost packets (ms)
- `--csv` : output CSV path
- `--seed` : RNG seed
- `--run` : RNG run index
- `--area` : square simulation area size (m)
- `--initialEnergyJ` : initial node energy (J)

### Example
```bash
./ns3 run "scratch/07-final-simulation.cc --protocol=AODV --errorRate=0.1 --nodes=20 --flows=12 --time=30 --csv=baseline.csv"
```

## 4.2 `scratch/08-extended-sim.cc`

### Supported arguments
- `--nodes`
- `--time`
- `--protocol` : `AODV`, `AOMDV`
- `--errorRate`
- `--minSpeed`
- `--speed`
- `--pause`
- `--flows`
- `--packetSize`
- `--interval`
- `--lossPenaltyMs`
- `--csv`
- `--seed`
- `--run`
- `--area`
- `--initialEnergyJ`

(Argument meanings are the same as in `07-final-simulation.cc`.)

### Example
```bash
./ns3 run "scratch/08-extended-sim.cc --protocol=AOMDV --errorRate=0.2 --nodes=20 --flows=10 --time=25 --csv=aodv-aomdv-results.csv"
```

## 4.3 `scratch/manet-unified-sim.cc`

### Supported arguments
- `--mode` : `wireless` or `wired`
- `--nodes` : number of nodes
- `--flows` : number of UDP flows
- `--pps` : packets per second per flow (must be > 0)
- `--interval` : per-flow interval in seconds (overrides `pps` if > 0)
- `--minSpeed` : wireless min speed (m/s)
- `--speed` : wireless max speed (m/s)
- `--pause` : random waypoint pause (s)
- `--time` : simulation time (s)
- `--area` : wireless area side length (m)
- `--packetSize` : UDP payload size (bytes)
- `--seed` : RNG seed
- `--run` : RNG run index
- `--trafficStart` : app traffic start time (s)
- `--csv` : output CSV path

### Notes
- `--mode=wired` uses CSMA + constant grid positions.
- `--mode=wireless` uses Wi-Fi ad hoc + random waypoint (if speed > 0).
- Validation checks include:
  - `nodes >= 2`
  - `pps > 0`
  - `trafficStart <= time - 0.5`

### Examples
```bash
./ns3 run "scratch/manet-unified-sim.cc --mode=wired --nodes=12 --flows=10 --pps=5 --time=60 --trafficStart=5 --csv=results/wired-csvs/manual-wired.csv"
```

```bash
./ns3 run "scratch/manet-unified-sim.cc --mode=wireless --nodes=20 --flows=20 --pps=10 --speed=15 --time=20 --csv=results/wireless-csvs/manual-wireless.csv"
```

## 5) Typical End-to-End Workflows

## A) Baseline AODV/DSDV/OLSR vs error rates
```bash
bash scratch/run-all-protocols.sh
```
Outputs:
- `baseline.csv`, `error10.csv`, `error20.csv`
- `comparison-graphs-e20/protocol_comparison.png`

## B) AODV vs AOMDV vs error rates
```bash
bash scratch/run-aodv-aomdv-all-errors.sh
```
Outputs:
- `aodv-aomdv-results.csv`
- `comparison-graphs-aodv-aomdv/aodv_aomdv_comparison.png`

## C) Wired AOMDV sweep
```bash
bash manet-unified-wired.sh
```
Outputs:
- `results/wired-csvs/*.csv`

## D) Wireless AOMDV sweep
```bash
bash manet-unified-wireless.sh
```
Outputs:
- `results/wireless-csvs/nn-test-speed.csv` (with current script)