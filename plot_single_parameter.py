#!/usr/bin/env python3

import csv
from pathlib import Path

import matplotlib.pyplot as plt


# =========================
# User-editable variables
# =========================
ROOT = Path(__file__).resolve().parent
CSV_PATH = ROOT / "results" / "wireless-csvs" / "nn-test-pps.csv"
OUTPUT_PATH = ROOT / "results" / "plot-single-parameter.png"

# Set the varying parameter column from your CSV.
# Examples: "Nodes", "Flows", "Pps", "Speed_mps"
X_COLUMN = "Pps"

PLOT_TITLE = "Single Parameter vs Metrics"

# Exactly 5 subplots are generated from these 5 metrics.
# Keep these aligned with headers in your CSV.
METRIC_COLUMNS = [
    "Throughput_bps",
    "AvgDelay_ms",
    "PDR",
    "DropRatio",
    "EnergyConsumed_J",
]

METRIC_LABELS = [
    "Throughput (bps)",
    "End-to-End Delay (ms)",
    "Packet Delivery Ratio",
    "Packet Drop Ratio",
    "Energy Consumption (J)",
]

FIGURE_SIZE = (10, 16)
DPI = 180


def to_float(value: str) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return 0.0


def read_rows(csv_path: Path):
    rows = []
    with csv_path.open("r", newline="") as file_obj:
        reader = csv.DictReader(file_obj)
        headers = reader.fieldnames or []
        for row in reader:
            rows.append(row)
    return headers, rows


def validate_config(headers):
    missing_columns = []
    if X_COLUMN not in headers:
        missing_columns.append(X_COLUMN)

    for col in METRIC_COLUMNS:
        if col not in headers:
            missing_columns.append(col)

    if missing_columns:
        missing_text = ", ".join(missing_columns)
        raise ValueError(f"Missing required columns in CSV: {missing_text}")


def main():
    if len(METRIC_COLUMNS) != 5 or len(METRIC_LABELS) != 5:
        raise ValueError("METRIC_COLUMNS and METRIC_LABELS must each contain exactly 5 items")

    if not CSV_PATH.exists():
        raise FileNotFoundError(f"CSV not found: {CSV_PATH}")

    headers, rows = read_rows(CSV_PATH)
    if not rows:
        raise ValueError(f"CSV is empty: {CSV_PATH}")

    validate_config(headers)

    rows.sort(key=lambda row: to_float(row.get(X_COLUMN, "0")))
    x_values = [to_float(row.get(X_COLUMN, "0")) for row in rows]

    fig, axes = plt.subplots(5, 1, figsize=FIGURE_SIZE, sharex=True)

    for axis, metric_col, metric_label in zip(axes, METRIC_COLUMNS, METRIC_LABELS):
        y_values = [to_float(row.get(metric_col, "0")) for row in rows]
        axis.plot(x_values, y_values, marker="o", linewidth=1.8)
        axis.set_ylabel(metric_label)
        axis.grid(True, linestyle="--", alpha=0.35)

    axes[-1].set_xlabel(X_COLUMN)
    fig.suptitle(PLOT_TITLE)
    fig.tight_layout(rect=[0, 0, 1, 0.98])

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(OUTPUT_PATH, dpi=DPI)
    plt.close(fig)

    print(f"Saved: {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
