#!/usr/bin/env python3
"""Plot wireless end-to-end delay with 4 subplots from unified simulation CSV."""

from __future__ import annotations

import argparse
import csv
import os
from collections import defaultdict
from statistics import mean


def to_num(v: str):
    if any(ch in v for ch in ".eE"):
        return float(v)
    return int(v)


def load_rows(path: str):
    rows = []
    with open(path, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            if row.get("Mode") != "wireless":
                continue
            parsed = {k: (row[k] if k == "Mode" else to_num(row[k])) for k in row}
            rows.append(parsed)
    if not rows:
        raise RuntimeError("No wireless rows found in CSV")
    return rows


def pick(rows, vary, fixed_nodes, fixed_flows, fixed_pps, fixed_speed):
    grouped = defaultdict(list)
    for r in rows:
        if vary != "Nodes" and int(r["Nodes"]) != fixed_nodes:
            continue
        if vary != "Flows" and int(r["Flows"]) != fixed_flows:
            continue
        if vary != "Pps" and int(r["Pps"]) != fixed_pps:
            continue
        if vary != "Speed_mps" and float(r["Speed_mps"]) != float(fixed_speed):
            continue
        grouped[r[vary]].append(float(r["AvgDelay_ms"]))

    x = sorted(grouped.keys())
    y = [mean(grouped[v]) for v in x]
    return x, y


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv", required=True)
    parser.add_argument("--out", default="results/wireless-delay-4subplots.png")
    parser.add_argument("--fixed-nodes", type=int, default=60)
    parser.add_argument("--fixed-flows", type=int, default=30)
    parser.add_argument("--fixed-pps", type=int, default=300)
    parser.add_argument("--fixed-speed", type=float, default=15)
    args = parser.parse_args()

    rows = load_rows(args.csv)

    import matplotlib.pyplot as plt

    fig, axs = plt.subplots(2, 2, figsize=(12, 8), constrained_layout=True)
    pairs = [
        ("Nodes", "Delay vs numNodes", "numNodes"),
        ("Flows", "Delay vs numFlows", "numFlows"),
        ("Pps", "Delay vs PPS", "PPS"),
        ("Speed_mps", "Delay vs nodeSpeed", "nodeSpeed (m/s)"),
    ]

    for ax, (col, title, xlabel) in zip(axs.flat, pairs):
        x, y = pick(rows, col, args.fixed_nodes, args.fixed_flows, args.fixed_pps, args.fixed_speed)
        ax.plot(x, y, marker="o", linewidth=2)
        ax.set_title(title)
        ax.set_xlabel(xlabel)
        ax.set_ylabel("End-to-End Delay (ms)")
        ax.grid(alpha=0.3)

    fig.suptitle(
        "Wireless AOMDV: End-to-End Delay (vary one parameter at a time)\n"
        f"fixed baseline: nodes={args.fixed_nodes}, flows={args.fixed_flows}, pps={args.fixed_pps}, speed={args.fixed_speed}",
        fontsize=12,
    )

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    fig.savefig(args.out, dpi=180)
    print(f"Saved plot: {args.out}")


if __name__ == "__main__":
    main()
