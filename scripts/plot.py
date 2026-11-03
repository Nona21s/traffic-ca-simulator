#!/usr/bin/env python3
"""Plot mean benchmark runtime by execution mode."""

import argparse
import csv
from collections import defaultdict
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv_file", type=Path, help="CSV written by traffic_benchmark")
    parser.add_argument("--output", type=Path, default=Path("results/benchmark.png"))
    args = parser.parse_args()

    times = defaultdict(list)
    with args.csv_file.open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            times[row["mode"]].append(float(row["elapsed_ms"]))

    if not times:
        raise SystemExit("The benchmark CSV contains no data rows.")

    try:
        import matplotlib.pyplot as plt
    except ImportError as error:
        raise SystemExit("Plotting requires matplotlib; install it with: python -m pip install matplotlib") from error

    modes = sorted(times)
    means = [sum(times[mode]) / len(times[mode]) for mode in modes]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    plt.bar(modes, means, color=["#4385c5" if mode == "sequential" else "#e18b42" for mode in modes])
    plt.ylabel("Mean runtime (ms)")
    plt.title("Traffic CA benchmark")
    plt.tight_layout()
    plt.savefig(args.output, dpi=160)
    print(f"Saved plot to {args.output}")


if __name__ == "__main__":
    main()
