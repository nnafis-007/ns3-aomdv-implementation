# AOMDV Implementation and AODV vs AOMDV Comparison Report

## 1. Overview

This report summarizes the implementation of AOMDV in ns-3.45 and presents benchmark results against AODV for packet error rates 0.0, 0.1, and 0.2.

The comparison was executed using:

- `scratch/08-extended-sim.cc`
- `scratch/run-aodv-aomdv-comparison.sh`

## 2. AOMDV Implementation Summary

The following AOMDV module components were implemented and integrated:

- `src/aomdv/CMakeLists.txt`
- `src/aomdv/model/aomdv-routing-protocol.h`
- `src/aomdv/model/aomdv-routing-protocol.cc`
- `src/aomdv/model/aomdv-rtable.h`
- `src/aomdv/model/aomdv-rtable.cc`
- `src/aomdv/model/aomdv-packet.h`
- `src/aomdv/model/aomdv-packet.cc`
- `src/aomdv/helper/aomdv-helper.h`
- `src/aomdv/helper/aomdv-helper.cc`

Key implementation points:

- Multipath routing table support (`AomdvPath` list per destination)
- AOMDV control packet support (RREQ/RREP/RERR with type dispatch)
- Route discovery and route maintenance logic
- RERR-based invalidation and propagation support
- Build integration via CMake and helper integration via `AomdvHelper`

## 3. Simulation Setup

- Protocols: `AODV`, `AOMDV`
- Error rates: `0.0`, `0.1`, `0.2`
- Seed: `1`
- Run index: `1`
- Output files:
	- `aodv-aomdv-err0.csv`
	- `aodv-aomdv-err01.csv`
	- `aodv-aomdv-err02.csv`

## 4. Results

Delta is computed as: `AOMDV - AODV`.

| Error Rate | Metric | AODV | AOMDV | Delta |
|---|---:|---:|---:|---:|
| 0.0 | PDR_pct | 92.31 | 79.55 | -12.76 |
| 0.0 | Throughput_kbps | 241.30 | 145.40 | -95.90 |
| 0.0 | AvgDelay_ms | 30.69 | 31.75 | +1.06 |
| 0.0 | DelayWithLossPenalty_ms | 606.80 | 308.90 | -297.90 |
| 0.0 | PacketLossRate_pct | 2.893 | 1.418 | -1.475 |
| 0.0 | RoutingOverhead | 0.1678 | 0.0366 | -0.1312 |
| 0.1 | PDR_pct | 75.62 | 76.01 | +0.39 |
| 0.1 | Throughput_kbps | 197.60 | 122.50 | -75.10 |
| 0.1 | AvgDelay_ms | 577.40 | 50.90 | -526.50 |
| 0.1 | DelayWithLossPenalty_ms | 3726.00 | 1620.00 | -2106.00 |
| 0.1 | PacketLossRate_pct | 16.45 | 7.909 | -8.541 |
| 0.1 | RoutingOverhead | 0.2331 | 0.0389 | -0.1942 |
| 0.2 | PDR_pct | 59.09 | 62.19 | +3.10 |
| 0.2 | Throughput_kbps | 154.40 | 92.02 | -62.38 |
| 0.2 | AvgDelay_ms | 538.20 | 84.47 | -453.73 |
| 0.2 | DelayWithLossPenalty_ms | 5260.00 | 1688.00 | -3572.00 |
| 0.2 | PacketLossRate_pct | 24.71 | 8.175 | -16.535 |
| 0.2 | RoutingOverhead | 0.0661 | 0.0642 | -0.0019 |

# Metrics to remove : Routing Overhead

## 5. Discussion of Improvements

Observed improvements for AOMDV are strongest at non-zero error rates:

- At `errorRate=0.1` and `0.2`, AOMDV shows:
	- Lower packet loss rate
	- Lower delay-related metrics (`AvgDelay_ms`, `DelayWithLossPenalty_ms`)
	- Slightly better PDR than AODV

Trade-off observed:

- Throughput (`Throughput_kbps`) remains lower for AOMDV in this configuration.

At `errorRate=0.0`:

- AODV performs better on PDR and throughput.
- AOMDV still shows better loss-penalty and loss-rate metrics.

## 6. Conclusion

The AOMDV module has been implemented and successfully integrated into ns-3.45. Comparative experiments against AODV were completed for error rates 0.0, 0.1, and 0.2.

In this setup, AOMDV demonstrates clear robustness improvements under lossy conditions, particularly in delay and packet-loss behavior, while AODV retains an advantage in throughput and baseline no-error PDR.
