#!/usr/bin/env python3
"""
Parse test.sh output files and generate formatted comparison tables.

Usage: python3 parse_results.py <output_file1> [output_file2] ...

Multiple output files are supported — results are averaged across files.
"""

import sys
import csv
from io import StringIO
from collections import defaultdict

# Order of test configurations as they appear in test.sh output.
REDIS_ORDER = [
    # (thp_policy, bgsave, memory_high)
    ("always",         "noBGSAVE", "max"),
    ("never",          "noBGSAVE", "max"),
    ("always+mthp_ext","noBGSAVE", "max"),
    ("always",         "BGSAVE",   "max"),
    ("never",          "BGSAVE",   "max"),
    ("always+mthp_ext","BGSAVE",   "max"),
    ("always",         "noBGSAVE", "2G"),
    ("never",          "noBGSAVE", "2G"),
    ("always+mthp_ext","noBGSAVE", "2G"),
    ("always",         "BGSAVE",   "2G"),
    ("never",          "BGSAVE",   "2G"),
    ("always+mthp_ext","BGSAVE",   "2G"),
]

UNIXBENCH_ORDER = [
    ("always",),
    ("never",),
    ("always+mthp_ext",),
]

REDIS_METRICS = ["rps", "avg_latency_ms", "p95_latency_ms", "p99_latency_ms"]

POLICY_LABELS = ["always", "never", "always+mthp_ext"]

# Metrics where higher values are better (pct = (val - base) / base).
# For others, lower is better (pct = (base - val) / base).
HIGHER_IS_BETTER = {"rps"}


def parse_file(filepath):
    """Extract redis CSV rows and unixbench scores from one output file."""
    with open(filepath, "r") as f:
        content = f.read()

    redis_rows = []
    unixbench_scores = []

    for line in content.splitlines():
        line = line.strip()
        if line.startswith('"SET"'):
            reader = csv.reader(StringIO(line))
            row = next(reader)
            redis_rows.append({
                "rps":             float(row[1]),
                "avg_latency_ms":  float(row[2]),
                "p95_latency_ms":  float(row[5]),
                "p99_latency_ms":  float(row[6]),
            })
        elif "System Benchmarks Index Score" in line:
            unixbench_scores.append(float(line.split()[-1]))

    return redis_rows, unixbench_scores


def mean(vals):
    return sum(vals) / len(vals)


def pct(base, val, metric, decimals=3):
    """Percentage change: positive = improvement, negative = regression.

    Values are rounded to `decimals` places first, so the percentage matches
    the displayed numbers.
    """
    base = round(base, decimals)
    val = round(val, decimals)
    diff = (val - base) if metric in HIGHER_IS_BETTER else (base - val)
    if base == 0:
        return 0.0
    return diff / base * 100


def fmt_val(val, decimals):
    return f"{val:.{decimals}f}"


def fmt_pct_str(p, decimals):
    return f"({p:.{decimals}f}%)"


def collect_results(files):
    """Read all files, group raw results by config key, return averages."""
    redis_raw = defaultdict(list)
    ubench_raw = defaultdict(list)

    for path in files:
        rr, ub = parse_file(path)

        if len(rr) != len(REDIS_ORDER):
            print(f"Warning: {path}: {len(rr)} redis rows (expected {len(REDIS_ORDER)})")
        if len(ub) != len(UNIXBENCH_ORDER):
            print(f"Warning: {path}: {len(ub)} unixbench scores (expected {len(UNIXBENCH_ORDER)})")

        for i, row in enumerate(rr):
            if i < len(REDIS_ORDER):
                redis_raw[REDIS_ORDER[i]].append(row)

        for i, score in enumerate(ub):
            if i < len(UNIXBENCH_ORDER):
                ubench_raw[UNIXBENCH_ORDER[i]].append(score)

    avg_redis = {}
    for key, rows in redis_raw.items():
        if rows:
            avg_redis[key] = {m: mean([r[m] for r in rows]) for m in REDIS_METRICS}

    avg_ubench = {}
    for key, scores in ubench_raw.items():
        if scores:
            avg_ubench[key] = mean(scores)

    return avg_redis, avg_ubench


def render_cell(val_str, pct_str, col_width):
    """Render a table cell of exactly col_width characters.

    Format: leading space, then value left-aligned and pct right-aligned
    within the remaining width.
    """
    inner = col_width - 1  # chars after mandatory leading space
    if pct_str:
        pad = inner - len(val_str) - len(pct_str)
        if pad < 1:
            pad = 1
        content = val_str + " " * pad + pct_str
    else:
        content = val_str.ljust(inner)
    return " " + content


def emit_redis_tables(avg_redis):
    title = "redis results"
    print(title)
    print("~" * len(title))
    print()
    print("command: redis-benchmark --csv -r 3000000 -n 3000000 -d 1024 -c 16 -P 32 -t set")
    print()

    for mem in ("max", "2G"):
        print(f"When cgroup memory.high={mem}.")
        print()

        for bgsave in ("noBGSAVE", "BGSAVE"):
            baseline = avg_redis.get(("always", bgsave, mem))
            if baseline is None:
                continue

            # Build natural-width cells: val parts and pct parts.
            val_parts = {m: [] for m in REDIS_METRICS}
            pct_parts = {m: [] for m in REDIS_METRICS}

            for metric in REDIS_METRICS:
                base_val = baseline[metric]
                for policy in POLICY_LABELS:
                    key = (policy, bgsave, mem)
                    val = avg_redis.get(key, {}).get(metric)
                    if val is None:
                        val_parts[metric].append("N/A")
                        pct_parts[metric].append("")
                    elif policy == "always":
                        val_parts[metric].append(fmt_val(val, 3))
                        pct_parts[metric].append("")
                    else:
                        p = pct(base_val, val, metric)
                        val_parts[metric].append(fmt_val(val, 3))
                        pct_parts[metric].append(fmt_pct_str(p, 1))

            # Compute column widths.
            label = f"redis-{bgsave}"
            metric_labels = {
                "rps": "rps",
                "avg_latency_ms": "avg_latency_ms",
                "p95_latency_ms": "p95_latency_ms",
                "p99_latency_ms": "p99_latency_ms",
            }
            # col 0: label column (include data row labels)
            col_w = [max(len(label), max(len(l) for l in metric_labels.values()))]
            # col 1-3: policy columns
            for ci in range(3):
                w = len(POLICY_LABELS[ci])
                for metric in REDIS_METRICS:
                    v = val_parts[metric][ci]
                    p = pct_parts[metric][ci]
                    natural = len(v) + (1 + len(p) if p else 0)
                    w = max(w, natural)
                col_w.append(w + 1)  # +1 for leading space

            def fmt_row(lbl, ci_val, ci_pct):
                line = "| " + lbl.ljust(col_w[0]) + " |"
                for ci in range(3):
                    line += render_cell(ci_val[ci], ci_pct[ci], col_w[ci + 1]) + " |"
                return line

            # Header
            empty = ["", "", ""]
            print(fmt_row(label, POLICY_LABELS, empty))
            # Separator with per-column dashes
            sep = "|" + "-" * (col_w[0] + 2) + "|"
            for w in col_w[1:]:
                sep += "-" * (w + 1) + "|"
            print(sep)
            # Data rows
            for metric in REDIS_METRICS:
                print(fmt_row(metric_labels[metric], val_parts[metric], pct_parts[metric]))

            print()


def emit_unixbench_table(avg_ubench):
    title = "unixbench results"
    print(title)
    print("~" * len(title))
    print()
    print("command: ./Run -c 1 shell8")
    print()

    baseline = avg_ubench.get(("always",))
    if baseline is None:
        print("(no data)")
        return

    val_parts = []
    pct_parts = []
    for policy in POLICY_LABELS:
        val = avg_ubench.get((policy,))
        if val is None:
            val_parts.append("N/A")
            pct_parts.append("")
        elif policy == "always":
            val_parts.append(f"{val:.1f}")
            pct_parts.append("")
        else:
            p = pct(baseline, val, "rps", decimals=1)  # score is higher-is-better
            val_parts.append(f"{val:.1f}")
            pct_parts.append(fmt_pct_str(p, 2))

    label = "unixbench shell8"
    col_w = [len(label)]
    for ci in range(3):
        v = val_parts[ci]
        p = pct_parts[ci]
        natural = len(v) + (1 + len(p) if p else 0)
        w = max(len(POLICY_LABELS[ci]), natural)
        col_w.append(w + 1)

    def fmt_row(lbl, ci_val, ci_pct):
        line = "| " + lbl.ljust(col_w[0]) + " |"
        for ci in range(3):
            line += render_cell(ci_val[ci], ci_pct[ci], col_w[ci + 1]) + " |"
        return line

    empty = ["", "", ""]
    print(fmt_row(label, POLICY_LABELS, empty))
    sep = "|" + "-" * (col_w[0] + 2) + "|"
    for w in col_w[1:]:
        sep += "-" * (w + 1) + "|"
    print(sep)
    print(fmt_row("Score", val_parts, pct_parts))
    print()


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 parse_results.py <output_file1> [output_file2] ...")
        sys.exit(1)

    avg_redis, avg_ubench = collect_results(sys.argv[1:])
    emit_redis_tables(avg_redis)
    emit_unixbench_table(avg_ubench)


if __name__ == "__main__":
    main()
