## Plan: Unified MANET Experiment Suite

Build one configurable ns-3 simulation workflow that evaluates AODV, AOMDV, DSDV, and OLSR in two separate modes:
1. Wired mode (static topology).
2. Wireless 802.11 MANET mode (mobile).

The plan includes full parameter sweeps, CSV generation, Python aggregation, and required plots for throughput, delay, PDR, drop ratio, and wireless energy consumption, with 3 runs per parameter point.

**Steps**
1. Lock experiment matrix and defaults:
1. Nodes: 20, 40, 60, 80, 100.
2. Flows: 10, 20, 30, 40, 50.
3. PPS per flow: 100, 200, 300, 400, 500.
4. Wireless speed: 5, 10, 15, 20, 25.
5. Runs: 1, 2, 3.
2. Create a unified simulator under scratch with CLI switches for mode, protocol, nodes, flows, pps, speed, seed, run, and output CSV. This blocks all later automation.
3. Reuse protocol-selection structure from 07-final-simulation.cc and 08-extended-sim.cc, adding all 4 protocol branches in one place.
4. Implement wired mode topology using CSMA for two node groups and one point-to-point interconnect between the groups (static nodes).
5. Implement wireless mode using ad-hoc Wi-Fi + mobility + energy model, reusing proven patterns from 07-final-simulation.cc.
6. Implement traffic generation with interval derived from PPS as $interval = 1 / pps$, keeping random src-dst flow assignment reproducible via seed/run.
7. Implement metric computation and CSV output for:
1. Throughput.
2. End-to-end delay.
3. PDR.
4. Packet drop ratio.
5. Energy consumption (wireless only).
8. Build a Python sweep runner to execute the full parameter matrix across protocols and runs, writing one raw CSV row per run.
9. Add Python aggregation to compute mean (and optional std) over 3 runs for each parameter point.
10. Build plotting script(s) that generate required graphs from aggregated CSV, separated by mode to keep wired/wireless semantics clean.
11. Run smoke validation on a reduced subset, then run the full campaign.

**Relevant files**
- 07-final-simulation.cc — primary reference for metrics and CSV structure.
- 08-extended-sim.cc — primary reference for AOMDV branch integration.
- aomdv-helper.h — AOMDV routing helper binding.
- aomdv-routing-protocol.cc — AOMDV protocol behavior reference.
- run-all-protocols.sh — existing sweep pattern to evolve.
- plot-manet-results.py — plotting baseline to extend.

**Verification**
1. Compile and run one wired + one wireless smoke test for all 4 protocols.
2. Validate CSV schema completeness and numeric sanity.
3. Run reduced sweep (single value per axis, all protocols, runs 1..3) and confirm correct aggregation row counts.
4. Generate all required plots and verify axes/units/legends.
5. Cross-check one wireless point against existing simulation outputs for reasonableness.

**Decisions captured**
- Two separate modes: wired and wireless (not a single hybrid run mode).
- PPS interpreted per flow.
- 3 repetitions per parameter point.
- Tx_range-based static coverage sweep excluded for this implementation path, per your decision.

If this plan looks right, approve and I’ll proceed to implementation in the next step.