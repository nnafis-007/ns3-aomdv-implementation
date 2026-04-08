#!/usr/bin/env python3
"""
Compare two MANET simulation CSVs side-by-side (e.g. no-error vs with-error).

Usage:
    python3 scratch/plot-error-comparison.py \\
        --csv1 manet-results.csv \\
        --csv2 aodv-manet-results.csv \\
        --outdir graphs

Optional labels (default = CSV filename):
    python3 scratch/plot-error-comparison.py \\
        --csv1 manet-results.csv  --label1 "No Error" \\
        --csv2 err-manet-results.csv --label2 "10% Error" \\
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
    print(f"Loaded {len(df)} rows from {path}  |  protocols: {list(df['Protocol'].unique())}")
    return df


def per_protocol_mean(df: pd.DataFrame) -> pd.DataFrame:
    """Average numeric columns per protocol (handles multiple runs of same config)."""
    return df.groupby("Protocol").mean(numeric_only=True)


# ── main plot ────────────────────────────────────────────────────────────────

METRICS = {
    "PDR_pct":            ("Packet Delivery Ratio", "%",    "skyblue",  "steelblue"),
    "Throughput_kbps":    ("Avg Throughput",        "kbps", "salmon",   "firebrick"),
    "AvgDelay_ms":        ("Avg E2E Delay",         "ms",   "lightgreen","seagreen"),
    "RoutingOverhead":    ("Routing Overhead",       "ratio","plum",     "purple"),
    "PacketLossRate_pct": ("Packet Loss Rate",       "%",    "gold",     "darkorange"),
}


def plot_comparison(df1: pd.DataFrame, df2: pd.DataFrame,
                    label1: str, label2: str, output_dir: str):
    """
    One subplot per metric.
    For each subplot: grouped bars — one bar-pair per protocol (label1 / label2).
    """
    g1 = per_protocol_mean(df1)
    g2 = per_protocol_mean(df2)

    # Union of protocols present in either CSV, sorted for consistency
    protocols = sorted(set(g1.index) | set(g2.index))
    n = len(protocols)
    x = np.arange(n)
    bar_w = 0.35

    fig, axes = plt.subplots(1, 5, figsize=(24, 6))
    fig.suptitle(
        f"MANET Metric Comparison:  {label1}  vs  {label2}",
        fontsize=15, fontweight="bold", y=1.01
    )

    for ax, (col, (title, unit, color1, color2)) in zip(axes, METRICS.items()):
        vals1 = [g1.loc[p, col] if p in g1.index else 0 for p in protocols]
        vals2 = [g2.loc[p, col] if p in g2.index else 0 for p in protocols]

        bars1 = ax.bar(x - bar_w / 2, vals1, bar_w,
                       label=label1, color=color1, edgecolor="black", linewidth=0.7)
        bars2 = ax.bar(x + bar_w / 2, vals2, bar_w,
                       label=label2, color=color2, edgecolor="black", linewidth=0.7)

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

        ax.set_title(f"{title}\n({unit})", fontsize=11, fontweight="bold")
        ax.set_xticks(x)
        ax.set_xticklabels(protocols, fontsize=10)
        ax.set_ylabel(unit, fontsize=9)
        ax.grid(axis="y", alpha=0.3, linestyle="--")
        ax.set_ylim(bottom=0)
        ax.spines["top"].set_visible(False)
        ax.spines["right"].set_visible(False)

    # Shared legend under the figure
    patch1 = mpatches.Patch(color="grey",     label=label1)
    patch2 = mpatches.Patch(color="dimgrey",  label=label2)
    # build proper legend from first axis patches
    handles = [
        mpatches.Patch(facecolor=list(METRICS.values())[0][2], edgecolor="black", label=label1),
        mpatches.Patch(facecolor=list(METRICS.values())[0][3], edgecolor="black", label=label2),
    ]
    fig.legend(handles=handles, loc="lower center", ncol=2,
               fontsize=12, frameon=True, bbox_to_anchor=(0.5, -0.06))

    plt.tight_layout()
    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, "protocol_comparison.png")
    plt.savefig(out_path, dpi=150, bbox_inches="tight")
    print(f"\nSaved: {out_path}")
    plt.close()


# ── console summary ──────────────────────────────────────────────────────────

def print_diff_table(df1: pd.DataFrame, df2: pd.DataFrame,
                     label1: str, label2: str):
    g1 = per_protocol_mean(df1)
    g2 = per_protocol_mean(df2)
    protocols = sorted(set(g1.index) | set(g2.index))

    cols = ["PDR_pct", "Throughput_kbps", "AvgDelay_ms",
            "RoutingOverhead", "PacketLossRate_pct"]
    col_w = 18

    header = f"{'Protocol':<10} {'Metric':<22} {label1:>{col_w}} {label2:>{col_w}} {'Δ (abs)':>{col_w}}"
    print("\n" + "=" * len(header))
    print(f"  {label1}  vs  {label2}")
    print("=" * len(header))
    print(header)
    print("-" * len(header))

    for proto in protocols:
        for col in cols:
            v1 = g1.loc[proto, col] if proto in g1.index else float("nan")
            v2 = g2.loc[proto, col] if proto in g2.index else float("nan")
            delta = v2 - v1
            sign  = "+" if delta >= 0 else ""
            print(f"{proto:<10} {col:<22} {v1:>{col_w}.4f} {v2:>{col_w}.4f} "
                  f"{sign}{delta:>{col_w-1}.4f}")
        print("-" * len(header))


# ── entry point ──────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Compare two MANET CSV results side-by-side."
    )
    parser.add_argument("--csv1",   required=True,
                        help="First CSV file (e.g. no-error baseline)")
    parser.add_argument("--csv2",   required=True,
                        help="Second CSV file (e.g. with error rate)")
    parser.add_argument("--label1", default=None,
                        help="Legend label for csv1 (default: csv1 filename)")
    parser.add_argument("--label2", default=None,
                        help="Legend label for csv2 (default: csv2 filename)")
    parser.add_argument("--outdir", default="comparison-graphs",
                        help="Output directory for the PNG (default: graphs/)")
    args = parser.parse_args()

    label1 = args.label1 or os.path.splitext(os.path.basename(args.csv1))[0]
    label2 = args.label2 or os.path.splitext(os.path.basename(args.csv2))[0]

    df1 = load(args.csv1)
    df2 = load(args.csv2)

    print_diff_table(df1, df2, label1, label2)
    plot_comparison(df1, df2, label1, label2, args.outdir)
    print("Done!")


if __name__ == "__main__":
    main()
