#!/usr/bin/env python3
"""
cgcat - Display cgroup resource data, default flat list (-t for tree).

Reads cgroup v2 files (default: memory.stat).
Patterns (-g, -k, -p) support regex via re.search.

Usage:
    cgcat                                   # flat list of all cgroups, all keys
    cgcat -k '^anon$'                       # exact key match
    cgcat -k 'anon|file|slab'               # regex on keys
    cgcat -k 'pgscan.*'                     # all pgscan variants
    cgcat -g 'sshd'                         # cgroups whose path contains sshd
    cgcat -g '^system.slice$'               # exact cgroup basename
    cgcat -g 'system.slice/.*'              # all under system.slice
    cgcat -p sshd                           # cgroups containing process sshd
    cgcat -p 'sshd|nginx'                   # regex on process name
    cgcat -f cpu.stat                       # read cpu.stat instead
    cgcat -d 3                              # limit tree depth
    cgcat -v '>:100M' -k anon               # values > 100M
    cgcat -e 'scale=$pgsteal/$pgscan'       # extra computed expression
    cgcat -e 'a=$anon/$file,b=$anon'        # multiple extras (comma-separated)
    cgcat -m 1 -k '^anon$'                  # monitor every 1s
    cgcat -t                                # tree display mode
"""

import argparse
import csv
import math
import os
import re
import shutil
import sys
import time
from datetime import datetime

import asciichartpy as acp

CGROUP_ROOT = "/sys/fs/cgroup"


def human_size(v):
    """Convert a raw byte value to a human-readable string."""
    if not isinstance(v, int):
        return str(v)
    if v < 1024:
        return str(v)
    for threshold, unit in [(1 << 40, 'T'), (1 << 30, 'G'), (1 << 20, 'M'), (1 << 10, 'K')]:
        if v >= threshold:
            val = v / threshold
            if val >= 100:
                return f"{int(val)}{unit}"
            if val >= 10:
                return f"{val:.0f}{unit}"
            return f"{val:.1f}{unit}"
    return str(v)


def parse_stat_file(path):
    """Parse a cgroup v2 key-value file into a dict."""
    d = {}
    try:
        with open(path) as f:
            for line in f:
                parts = line.split()
                if len(parts) >= 2:
                    try:
                        d[parts[0]] = int(parts[1])
                    except ValueError:
                        d[parts[0]] = parts[1]
    except OSError:
        pass
    return d


def build_tree(root, max_depth):
    """Walk cgroup root and return a nested dict representing the tree."""
    def walk(path, depth):
        node = {'name': os.path.basename(path) or '/',
                'path': path,
                'children': [],
                'depth': depth}
        if max_depth is not None and depth >= max_depth:
            return node
        try:
            entries = sorted(os.listdir(path))
        except OSError:
            return node
        for name in entries:
            child_path = os.path.join(path, name)
            if os.path.isdir(child_path):
                node['children'].append(walk(child_path, depth + 1))
        return node
    return walk(root, 0)


def collect_flat(root, max_depth):
    """Return a flat list of cgroup dirs under root, sorted by depth then path."""
    dirs = []
    for dirpath, subdirs, _ in os.walk(root):
        depth = dirpath.replace(root, '').count(os.sep)
        if max_depth is not None and depth > max_depth:
            subdirs.clear()
            continue
        dirs.append(dirpath)
        subdirs.sort()
    dirs.sort(key=lambda d: (d.replace(root, '').count(os.sep), d))
    return dirs


def match_cgroup(dirs, pattern):
    """Filter dirs by regex on relative path from cgroup root (no leading /)."""
    cg_re = re.compile(pattern, re.IGNORECASE)
    return [d for d in dirs if cg_re.search(
        d.replace(CGROUP_ROOT, '').lstrip('/') or '/')]


def format_stat(stat, key_res, value_filter, extras=None):
    """Filter and format stat dict entries into a display string."""
    vals = []
    for k, v in stat.items():
        if key_res is not None and not any(r.search(k) for r in key_res):
            continue
        if isinstance(v, int) and value_filter:
            if not value_filter(v):
                continue
        vals.append(f"{k}={human_size(v)}")
    if extras:
        for name, expr in extras:
            vals.append(f"{name}={compute_extra_expr(expr, stat)}")
    return ", ".join(vals) if vals else "(no data)"


def mark_tree_visible(node, cgroup_re):
    """Mark node.visible = True if node or any descendant matches cgroup_re."""
    rel = node['path'].replace(CGROUP_ROOT, '').lstrip('/') or '/'
    node['visible'] = bool(cgroup_re.search(rel))
    for child in node['children']:
        if mark_tree_visible(child, cgroup_re):
            node['visible'] = True
    return node['visible']


def mark_tree_by_paths(node, path_set, parent_matched=False):
    """Mark visible nodes: those in path_set, plus their ancestors and descendants."""
    matched = node['path'] in path_set or parent_matched
    node['visible'] = matched
    for child in node['children']:
        if mark_tree_by_paths(child, path_set, matched):
            node['visible'] = True
    return node['visible']


def print_tree(node, data_file, key_res, value_filter, extras=None, cgroup_re=None,
               path_set=None, prefix="", is_last=True):
    """Recursively print a cgroup tree with data."""
    use_filter = cgroup_re or path_set
    visible_children = [c for c in node['children'] if c.get('visible', True)] if use_filter else node['children']

    if not visible_children and use_filter and not node.get('visible'):
        return

    branch = "└── " if is_last else "├── "
    connector = "    " if is_last else "│   "

    stat_path = os.path.join(node['path'], data_file)
    stat = parse_stat_file(stat_path)
    val_str = format_stat(stat, key_res, value_filter, extras)

    name = node['name']
    print(f"{prefix}{branch}{name}  {val_str}")

    for i, child in enumerate(visible_children):
        print_tree(child, data_file, key_res, value_filter, extras, cgroup_re, path_set,
                   prefix + connector, i == len(visible_children) - 1)


def print_plain(dirs, data_file, key_res, value_filter, extras=None):
    """Print a flat listing with full paths."""
    for d in dirs:
        stat_path = os.path.join(d, data_file)
        stat = parse_stat_file(stat_path)
        name = d.replace(CGROUP_ROOT, '') or '/'
        val_str = format_stat(stat, key_res, value_filter, extras)
        print(f"{name}  {val_str}")


def parse_value_filter(raw):
    """Parse a value filter string like '>:1M' into a callable."""
    m = re.match(r'([<>=]+)\s*:\s*(.+)', raw)
    if not m:
        sys.exit(f"Invalid filter format: {raw} (expected OP:VAL, e.g. '>:1M')")

    op = m.group(1)
    val_str = m.group(2).strip().upper()

    mult = 1
    for suffix, factor in [('T', 1 << 40), ('G', 1 << 30), ('M', 1 << 20), ('K', 1 << 10)]:
        if val_str.endswith(suffix):
            mult = factor
            val_str = val_str[:-1]
            break

    try:
        threshold = int(float(val_str) * mult)
    except ValueError:
        sys.exit(f"Invalid filter value: {val_str}")

    ops = {
        '>':  lambda v: v > threshold,
        '<':  lambda v: v < threshold,
        '>=': lambda v: v >= threshold,
        '<=': lambda v: v <= threshold,
        '=':  lambda v: v == threshold,
    }
    if op not in ops:
        sys.exit(f"Invalid filter operator: {op} (expected >, <, >=, <=, =)")
    return ops[op]


def compute_extra_expr(expr, stat):
    """Evaluate expr like '$pgsteal/$pgscan' -> '0.68'."""
    def sub_var(m):
        val = stat.get(m.group(1), 0)
        return str(val) if isinstance(val, int) else '0'

    expanded = re.sub(r'\$([a-zA-Z_]\w*)', sub_var, expr).strip()
    try:
        result = eval(expanded, {'__builtins__': {}})
    except ZeroDivisionError:
        return 'N/A'
    except Exception:
        return expanded

    if isinstance(result, float):
        return f"{result:.2f}"
    return str(result)


def parse_extra_args(raw):
    """Parse '-e' string into [(name, expr_template), ...]."""
    extras = []
    for item in raw.split(','):
        item = item.strip()
        if not item:
            continue
        if '=' not in item:
            sys.exit(f"Invalid extra format: {item} (expected NAME=EXPR)")
        name, expr = item.split('=', 1)
        extras.append((name.strip(), expr.strip()))
    return extras


def resolve_proc_cgroups(pattern):
    """Find cgroups containing processes whose comm matches the regex pattern.
    Returns list of (cgroup_path, [pids]) sorted by path."""
    comm_re = re.compile(pattern, re.IGNORECASE)
    cgroups = {}
    try:
        for entry in os.listdir('/proc'):
            if not entry.isdigit():
                continue
            try:
                with open(f'/proc/{entry}/comm') as f:
                    comm = f.read().strip()
            except OSError:
                continue
            if not comm_re.search(comm):
                continue
            try:
                with open(f'/proc/{entry}/cgroup') as f:
                    for line in f:
                        parts = line.strip().split(':')
                        if len(parts) == 3 and parts[0] == '0' and parts[1] == '':
                            rel = parts[2].lstrip('/')
                            cg = os.path.join(CGROUP_ROOT, rel) if rel else CGROUP_ROOT
                            cgroups.setdefault(cg, []).append(int(entry))
                            break
            except OSError:
                continue
    except OSError:
        pass
    if not cgroups:
        sys.exit(f"No process found matching '{pattern}'")
    return sorted(cgroups.items(), key=lambda x: x[0])


def monitor_loop(dirs, data_file, key_res, value_filter, extras, interval):
    """Repeatedly read and display cgroup data as a dynamic line chart, collect rows for CSV."""
    rows = []
    series_history = {}  # label -> [value per tick]
    t0 = time.time()
    tick = 0

    is_dark_bg = True  # default
    fgbg = os.environ.get('COLORFGBG', '')
    if ';' in fgbg:
        try:
            _, bg = fgbg.split(';')
            is_dark_bg = int(bg) < 8
        except ValueError:
            pass

    def series_color(label):
        """Deterministic color: bright for dark terminals, dark for light terminals."""
        h = hash(label)
        if is_dark_bg:
            r = 2 + (h % 4)       # 2..5, brighter half of 6×6×6 cube
            g = 2 + ((h >> 4) % 4)
            b = 2 + ((h >> 8) % 4)
        else:
            r = h % 4             # 0..3, darker half
            g = (h >> 4) % 4
            b = (h >> 8) % 4
        code = 16 + 36 * r + 6 * g + b
        return f"\033[38;5;{code}m"

    try:
        while True:
            now = datetime.now()
            timestamp = now.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]

            tick_snapshot = {}

            for d in dirs:
                stat = parse_stat_file(os.path.join(d, data_file))
                cg_name = os.path.basename(d) or '/'
                row = {'timestamp': timestamp, 'cgroup': cg_name}

                for k, v in stat.items():
                    if key_res is not None and not any(r.search(k) for r in key_res):
                        continue
                    if isinstance(v, int):
                        row[k] = v
                        if not value_filter or value_filter(v):
                            tick_snapshot[f"{cg_name}:{k}"] = v

                if extras:
                    for name, expr in extras:
                        val = compute_extra_expr(expr, stat)
                        try:
                            tick_snapshot[f"{cg_name}:{name}"] = float(val)
                        except ValueError:
                            pass
                        try:
                            row[name] = float(val)
                        except ValueError:
                            row[name] = val

                rows.append(row)

            for key in series_history:
                series_history[key].append(tick_snapshot.get(key))
            for key in tick_snapshot:
                if key not in series_history:
                    series_history[key] = [None] * tick + [tick_snapshot[key]]

            # Build chart data: convert None to NaN, put legend on the right
            plot_data = []
            plot_colors = []
            labels = []
            for label, values in series_history.items():
                clean = [v if v is not None else float('nan') for v in values]
                if any(not math.isnan(v) for v in clean):
                    plot_data.append(clean)
                    plot_colors.append(series_color(label))
                    labels.append(label)

            term = shutil.get_terminal_size()
            print("\033[2J\033[H", end="")
            print(f"cgcat -m {interval}s | {timestamp} | tick={tick} | Ctrl-C to save\n")

            if plot_data and term.columns >= 40 and term.lines >= 10:
                chart_height = max(6, min(20, term.lines - 6))

                # Sliding window: keep recent points, leave room for legend on the right
                max_label_len = max((len(lbl) for lbl in labels), default=0)
                max_points = max(10, term.columns - 16 - max_label_len)
                plot_data_windowed = []
                for series in plot_data:
                    if len(series) > max_points:
                        plot_data_windowed.append(series[-max_points:])
                    else:
                        plot_data_windowed.append(series)

                # Compute global Y range with padding so few data points still fill the chart
                all_vals = [v for series in plot_data for v in series if not math.isnan(v)]
                if all_vals:
                    y_min = min(all_vals)
                    y_max = max(all_vals)
                    # At least 5 % of max absolute value, or 10 % of the span
                    y_pad = max((y_max - y_min) * 0.1,
                                max(abs(y_max), abs(y_min)) * 0.05, 1)
                    c_min = y_min - y_pad
                    c_max = y_max + y_pad
                    if min(all_vals) >= 0:
                        c_min = max(0, c_min)
                else:
                    c_min, c_max = 0, 1

                chart = acp.plot(plot_data_windowed, {
                    'height': chart_height,
                    'colors': plot_colors,
                    'format': '{:>10.0f}',
                    'min': c_min,
                    'max': c_max,
                })

                # Print chart with legend overlaid on the right side
                chart_lines = chart.rstrip().split('\n')
                legend_col = max(14 + max_points, term.columns - max_label_len - 1)
                for i, line in enumerate(chart_lines):
                    print(line, end='')
                    if i < len(labels):
                        print(f"\033[{legend_col}G", end='')
                        print(acp.colored(labels[i], series_color(labels[i])), end='')
                    print()
                for i in range(len(chart_lines), len(labels)):
                    print(' ' * (legend_col - 1)
                          + acp.colored(labels[i], series_color(labels[i])))
            else:
                if term.columns < 40 or term.lines < 10:
                    print(f"(terminal too small: {term.columns}x{term.lines})")
                for d in dirs:
                    stat_path = os.path.join(d, data_file)
                    stat = parse_stat_file(stat_path)
                    name = d.replace(CGROUP_ROOT, '') or '/'
                    val_str = format_stat(stat, key_res, value_filter, extras)
                    print(f"{name}  {val_str}")

            sys.stdout.flush()
            tick += 1

            elapsed = time.time() - t0
            sleep_for = interval - (elapsed % interval)
            if sleep_for > 0:
                time.sleep(sleep_for)
    except KeyboardInterrupt:
        print()
        write_csv(rows, key_res)


def write_csv(rows, key_res):
    """Write collected monitoring data to a CSV file."""
    if not rows:
        print("\nNo data collected.")
        return

    columns = ['timestamp', 'cgroup']
    key_set = set()
    for row in rows:
        key_set.update(k for k in row if k not in columns)
    columns.extend(sorted(key_set))

    filename = f"cgcat_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    with open(filename, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=columns, extrasaction='ignore')
        writer.writeheader()
        writer.writerows(rows)

    print(f"\nSaved {len(rows)} rows to {filename}")

    html_path = filename.replace('.csv', '.html')
    write_plotly_chart(rows, html_path)


def write_plotly_chart(rows, html_path):
    """Generate an interactive plotly HTML chart from monitoring data."""
    try:
        import plotly.graph_objects as go
    except ImportError:
        return

    cg_data = {}
    for row in rows:
        cg = row['cgroup']
        if cg not in cg_data:
            cg_data[cg] = {'timestamps': [], 'keys': {}}
        cg_data[cg]['timestamps'].append(row['timestamp'])
        for k, v in row.items():
            if k in ('timestamp', 'cgroup'):
                continue
            if k not in cg_data[cg]['keys']:
                cg_data[cg]['keys'][k] = []
            cg_data[cg]['keys'][k].append(v if isinstance(v, (int, float)) else None)

    fig = go.Figure()
    for cg, data in cg_data.items():
        for key, values in data['keys'].items():
            if any(v is not None for v in values):
                hover_vals = [human_size(v) if v is not None else 'N/A' for v in values]
                fig.add_trace(go.Scatter(
                    x=data['timestamps'], y=values,
                    mode='lines+markers', name=f"{cg}:{key}",
                    hovertemplate="%{customdata}<extra></extra>",
                    customdata=hover_vals
                ))

    fig.update_layout(
        title='cgcat Monitor',
        xaxis_title='Time', yaxis_title='Value',
        hovermode='x unified', legend=dict(font=dict(size=10)),
        yaxis=dict(exponentformat='none', separatethousands=True)
    )
    fig.write_html(html_path)
    print(f"Chart saved to {html_path}")


def main():
    parser = argparse.ArgumentParser(
        description='Display cgroup resource data')
    parser.add_argument('-g', '--cgroup', default=None,
                        help='Show only cgroups matching this path segment')
    parser.add_argument('-p', '--process', default=None,
                        help='Show cgroups containing processes matching name')
    parser.add_argument('-f', '--file', default='memory.stat',
                        help='Cgroup data file to read (default: memory.stat)')
    parser.add_argument('-k', '--keys', default=None,
                        help='Comma-separated keys to display (default: all keys)')
    parser.add_argument('-d', '--depth', type=int, default=None,
                        help='Maximum tree depth (default: unlimited)')
    parser.add_argument('-v', '--filter', default=None, metavar='OP:VAL',
                        help='Filter keys by value, e.g. >:1M, <:1G, >=:1048576')
    parser.add_argument('-e', '--extra', default=None, metavar='NAME=EXP',
                        help='Extra computed expressions, e.g. scale=$a/$b')
    parser.add_argument('-m', '--monitor', type=float, default=None, metavar='SEC',
                        help='Monitor mode, refresh every SEC seconds')
    parser.add_argument('-t', '--tree', action='store_true',
                        help='Tree display mode')
    args = parser.parse_args()

    if args.keys:
        key_res = [re.compile(p.strip(), re.IGNORECASE) for p in args.keys.split(',')]
    else:
        key_res = None  # default: show all

    # Parse value filter and extra expressions
    value_filter = parse_value_filter(args.filter) if args.filter else None
    extras = parse_extra_args(args.extra) if args.extra else None

    if not os.path.isdir(CGROUP_ROOT):
        sys.exit(f"Error: {CGROUP_ROOT} not found. Is cgroup v2 mounted?")

    # Monitor mode
    if args.monitor:
        if args.process:
            dirs = [cg_path for cg_path, _ in resolve_proc_cgroups(args.process)]
        elif args.cgroup:
            dirs = collect_flat(CGROUP_ROOT, None)
            dirs = match_cgroup(dirs, args.cgroup)
            if not dirs:
                sys.exit(f"No cgroup found matching '{args.cgroup}'")
        else:
            dirs = collect_flat(CGROUP_ROOT, None)
        monitor_loop(dirs, args.file, key_res, value_filter, extras,
                     args.monitor)
        return

    # Determine root path
    if args.process:
        cgroups = resolve_proc_cgroups(args.process)
        path_set = {cg_path for cg_path, _ in cgroups}
        if args.tree:
            root = build_tree(CGROUP_ROOT, args.depth)
            mark_tree_by_paths(root, path_set)
            print_tree(root, args.file, key_res, value_filter, extras, path_set=path_set)
        else:
            for cg_path in sorted(path_set):
                stat = parse_stat_file(os.path.join(cg_path, args.file))
                name = cg_path.replace(CGROUP_ROOT, '') or '/'
                val_str = format_stat(stat, key_res, value_filter, extras)
                print(f"{name}  {val_str}")
        return
    elif args.cgroup:
        cg_re = re.compile(args.cgroup, re.IGNORECASE)
        if args.tree:
            root = build_tree(CGROUP_ROOT, args.depth)
            mark_tree_visible(root, cg_re)
            if not root['visible']:
                sys.exit(f"No cgroup found matching '{args.cgroup}'")
            print_tree(root, args.file, key_res, value_filter, extras, cg_re)
        else:
            dirs = collect_flat(CGROUP_ROOT, args.depth)
            dirs = match_cgroup(dirs, args.cgroup)
            if not dirs:
                sys.exit(f"No cgroup found matching '{args.cgroup}'")
            print_plain(dirs, args.file, key_res, value_filter, extras)
        return
    else:
        if args.tree:
            root = build_tree(CGROUP_ROOT, args.depth)
            print_tree(root, args.file, key_res, value_filter, extras)
        else:
            dirs = collect_flat(CGROUP_ROOT, args.depth)
            print_plain(dirs, args.file, key_res, value_filter, extras)


if __name__ == '__main__':
    main()
