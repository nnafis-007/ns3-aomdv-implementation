#!/usr/bin/env python3

import csv
from pathlib import Path

import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parent


def read_rows(csv_path: Path):
    rows = []
    with csv_path.open("r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(row)
    return rows


def to_float(value: str) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return 0.0


def plot_parameter(csv_path: Path, x_col: str, metrics, title: str, output_path: Path):
    rows = read_rows(csv_path)
    if not rows:
        print(f"Skipping empty CSV: {csv_path}")
        return

    rows.sort(key=lambda r: to_float(r.get(x_col, "0")))
    x_vals = [to_float(r.get(x_col, "0")) for r in rows]

    fig, axes = plt.subplots(len(metrics), 1, figsize=(10, 3.4 * len(metrics)), sharex=True)
    if len(metrics) == 1:
        axes = [axes]

    for ax, (col, ylabel) in zip(axes, metrics):
        y_vals = [to_float(r.get(col, "0")) for r in rows]
        ax.plot(x_vals, y_vals, marker="o", linewidth=1.8)
        ax.set_ylabel(ylabel)
        ax.grid(True, linestyle="--", alpha=0.35)

    axes[-1].set_xlabel(x_col)
    fig.suptitle(title)
    fig.tight_layout(rect=[0, 0, 1, 0.98])
    fig.savefig(output_path, dpi=180)
    plt.close(fig)
    print(f"Saved: {output_path}")


def main():
    wired_dir = ROOT / "results" / "wired-csvs"
    wireless_dir = ROOT / "results" / "wireless-csvs"

    wired_metrics = [
        ("Throughput_bps", "Throughput (bps)"),
        ("AvgDelay_ms", "End-to-End Delay (ms)"),
        ("PDR", "Packet Delivery Ratio"),
        ("DropRatio", "Packet Drop Ratio"),
    ]

    wireless_metrics = [
        ("Throughput_bps", "Throughput (bps)"),
        ("AvgDelay_ms", "End-to-End Delay (ms)"),
        ("PDR", "Packet Delivery Ratio"),
        ("DropRatio", "Packet Drop Ratio"),
        ("EnergyConsumed_J", "Energy Consumption (J)"),
    ]

    wired_plots = [
        (wired_dir / "nn-test-nodes.csv", "Nodes", wired_metrics, wired_dir / "plot-nodes-metrics.png"),
        (wired_dir / "nn-test-flows.csv", "Flows", wired_metrics, wired_dir / "plot-flows-metrics.png"),
        (wired_dir / "nn-test-pps.csv", "Pps", wired_metrics, wired_dir / "plot-pps-metrics.png"),
    ]

    wireless_plots = [
        (wireless_dir / "nn-test-nodes.csv", "Nodes", wireless_metrics, wireless_dir / "plot-nodes-metrics.png"),
        (wireless_dir / "nn-test-flows.csv", "Flows", wireless_metrics, wireless_dir / "plot-flows-metrics.png"),
        (wireless_dir / "nn-test-pps.csv", "Pps", wireless_metrics, wireless_dir / "plot-pps-metrics.png"),
        (wireless_dir / "nn-test-speed.csv", "Speed_mps", wireless_metrics, wireless_dir / "plot-speed-metrics.png"),
    ]

    for csv_path, x_col, metrics, output_path in wired_plots:
        if csv_path.exists():
            plot_parameter(
                csv_path,
                x_col,
                metrics,
                f"Wired: {x_col} vs Target Metrics",
                output_path,
            )
        else:
            print(f"Missing CSV: {csv_path}")

    for csv_path, x_col, metrics, output_path in wireless_plots:
        if csv_path.exists():
            plot_parameter(
                csv_path,
                x_col,
                metrics,
                f"Wireless: {x_col} vs Target Metrics",
                output_path,
            )
        else:
            print(f"Missing CSV: {csv_path}")


if __name__ == "__main__":
    main()