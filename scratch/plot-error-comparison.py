#!/usr/bin/env python3
"""
Compare three MANET simulation CSVs side-by-side (e.g. 0%, 10%, 20% error).

Usage:
    python3 scratch/plot-error-comparison.py \\
        --csv1 baseline.csv \\
        --csv2 error10.csv \\
        --csv3 error20.csv \\
        --outdir graphs

Optional labels (default = CSV filename):
    python3 scratch/plot-error-comparison.py \\
        --csv1 baseline.csv --label1 "0% Error" \\
        --csv2 error10.csv  --label2 "10% Error" \\
        --csv3 error20.csv  --label3 "20% Error" \\
        --outdir graphs
"""

import argparse
import os
import sys

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
import pandas as pd


# ── helpers ─────────────────────────────────────────────────────────────────

def load(path: str) -> pd.DataFrame:
    if not os.path.exists(path):
        print(f"ERROR: file not found: {path}")
        sys.exit(1)
    df = pd.read_csv(path)
    if "avgDelay" not in df.columns:
        if "DelayWithPenalty_ms" in df.columns:
            df = df.rename(columns={"DelayWithPenalty_ms": "avgDelay"})
        elif "DelayWithLossPenalty_ms" in df.columns:
            df = df.rename(columns={"DelayWithLossPenalty_ms": "avgDelay"})
    print(f"Loaded {len(df)} rows from {path}  |  protocols: {list(df['Protocol'].unique())}")
    return df


def per_protocol_mean(df: pd.DataFrame) -> pd.DataFrame:
    """Average numeric columns per protocol (handles multiple runs of same config)."""
    return df.groupby("Protocol").mean(numeric_only=True)


# ── main plot ────────────────────────────────────────────────────────────────

METRICS = {
    "PDR_pct":            ("Packet Delivery Ratio", "%",    "skyblue",  "steelblue"),
    "Throughput_kbps":    ("Avg Throughput",        "kbps", "salmon",   "firebrick"),
    "avgDelay":           ("Avg Delay",             "ms",   "lightgreen","seagreen"),
    "RoutingOverhead":    ("Routing Overhead",       "ratio","plum",     "purple"),
    "PacketLossRate_pct": ("Packet Loss Rate",       "%",    "gold",     "darkorange"),
}


def plot_comparison(df1: pd.DataFrame, df2: pd.DataFrame, df3: pd.DataFrame,
                    label1: str, label2: str, label3: str, output_dir: str):
    """
    One subplot per metric.
    For each subplot: grouped bars — one 3-bar group per protocol.
    """
    g1 = per_protocol_mean(df1)
    g2 = per_protocol_mean(df2)
    g3 = per_protocol_mean(df3)

    # Union of protocols present in either CSV, sorted for consistency
    protocols = sorted(set(g1.index) | set(g2.index) | set(g3.index))
    n = len(protocols)
    x = np.arange(n)
    bar_w = 0.25

    fig, axes = plt.subplots(1, 5, figsize=(24, 6))
    fig.suptitle(
        f"MANET Metric Comparison: {label1} vs {label2} vs {label3}",
        fontsize=15, fontweight="bold", y=1.01
    )

    for ax, (col, (title, unit, color1, color2)) in zip(axes, METRICS.items()):
        vals1 = [g1.loc[p, col] if p in g1.index else 0 for p in protocols]
        vals2 = [g2.loc[p, col] if p in g2.index else 0 for p in protocols]
        vals3 = [g3.loc[p, col] if p in g3.index else 0 for p in protocols]

        bars1 = ax.bar(x - bar_w, vals1, bar_w,
                       label=label1, color=color1, edgecolor="black", linewidth=0.7)
        bars2 = ax.bar(x, vals2, bar_w,
                       label=label2, color=color2, edgecolor="black", linewidth=0.7)
        bars3 = ax.bar(x + bar_w, vals3, bar_w,
                       label=label3, color="mediumorchid", edgecolor="black", linewidth=0.7)

        # value labels on top of each bar
        for bar, val in zip(bars1, vals1):
            if val > 0:
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + ax.get_ylim()[1] * 0.01,
                        f"{val:.1f}", ha="center", va="bottom",
                        fontsize=8, color="black")
        for bar, val in zip(bars2, vals2):
            if val > 0:
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + ax.get_ylim()[1] * 0.01,
                        f"{val:.1f}", ha="center", va="bottom",
                        fontsize=8, color="black")
        for bar, val in zip(bars3, vals3):
            if val > 0:
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + ax.get_ylim()[1] * 0.01,
                        f"{val:.1f}", ha="center", va="bottom",
                        fontsize=8, color="black")

        ax.set_title(f"{title}\n({unit})", fontsize=11, fontweight="bold")
        ax.set_xticks(x)
        ax.set_xticklabels(protocols, fontsize=10)
        ax.set_ylabel(unit, fontsize=9)
        ax.grid(axis="y", alpha=0.3, linestyle="--")
        ax.set_ylim(bottom=0)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)

    # Shared legend under the figure
    handles = [
        mpatches.Patch(facecolor=list(METRICS.values())[0][2], edgecolor="black", label=label1),
        mpatches.Patch(facecolor=list(METRICS.values())[0][3], edgecolor="black", label=label2),
        mpatches.Patch(facecolor="mediumorchid", edgecolor="black", label=label3),
    ]
    fig.legend(handles=handles, loc="lower center", ncol=3,
               fontsize=12, frameon=True, bbox_to_anchor=(0.5, -0.06))

    plt.tight_layout()
    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, "protocol_comparison.png")
    plt.savefig(out_path, dpi=150, bbox_inches="tight")
    print(f"\nSaved: {out_path}")
    plt.close()


# ── console summary ──────────────────────────────────────────────────────────

def print_diff_table(df1: pd.DataFrame, df2: pd.DataFrame, df3: pd.DataFrame,
                     label1: str, label2: str, label3: str):
    g1 = per_protocol_mean(df1)
    g2 = per_protocol_mean(df2)
    g3 = per_protocol_mean(df3)
    protocols = sorted(set(g1.index) | set(g2.index) | set(g3.index))

    cols = ["PDR_pct", "Throughput_kbps", "avgDelay",
            "RoutingOverhead", "PacketLossRate_pct"]
    col_w = 14

    header = (
        f"{'Protocol':<10} {'Metric':<22} {label1:>{col_w}} {label2:>{col_w}} {label3:>{col_w}} "
        f"{'Δ2-1':>{col_w}} {'Δ3-1':>{col_w}}"
    )
    print("\n" + "=" * len(header))
    print(f"  {label1}  vs  {label2}  vs  {label3}")
    print("=" * len(header))
    print(header)
    print("-" * len(header))

    for proto in protocols:
        for col in cols:
            v1 = g1.loc[proto, col] if proto in g1.index else float("nan")
            v2 = g2.loc[proto, col] if proto in g2.index else float("nan")
            v3 = g3.loc[proto, col] if proto in g3.index else float("nan")
            d21 = v2 - v1
            d31 = v3 - v1
            print(
                f"{proto:<10} {col:<22} {v1:>{col_w}.4f} {v2:>{col_w}.4f} {v3:>{col_w}.4f} "
                f"{d21:>{col_w}.4f} {d31:>{col_w}.4f}"
            )
        print("-" * len(header))


# ── entry point ──────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Compare three MANET CSV results side-by-side."
    )
    parser.add_argument("--csv1",   required=True,
                        help="First CSV file (e.g. 0% error baseline)")
    parser.add_argument("--csv2",   required=True,
                        help="Second CSV file (e.g. 10% error)")
    parser.add_argument("--csv3",   required=True,
                        help="Third CSV file (e.g. 20% error)")
    parser.add_argument("--label1", default=None,
                        help="Legend label for csv1 (default: csv1 filename)")
    parser.add_argument("--label2", default=None,
                        help="Legend label for csv2 (default: csv2 filename)")
    parser.add_argument("--label3", default=None,
                        help="Legend label for csv3 (default: csv3 filename)")
    parser.add_argument("--outdir", default="comparison-graphs",
                        help="Output directory for the PNG (default: graphs/)")
    args = parser.parse_args()

    label1 = args.label1 or os.path.splitext(os.path.basename(args.csv1))[0]
    label2 = args.label2 or os.path.splitext(os.path.basename(args.csv2))[0]
    label3 = args.label3 or os.path.splitext(os.path.basename(args.csv3))[0]

    df1 = load(args.csv1)
    df2 = load(args.csv2)
    df3 = load(args.csv3)

    print_diff_table(df1, df2, df3, label1, label2, label3)
    plot_comparison(df1, df2, df3, label1, label2, label3, args.outdir)
    print("Done!")


if __name__ == "__main__":
    main()
