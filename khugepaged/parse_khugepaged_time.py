#!/usr/bin/env python3
import re
import sys
import subprocess
import signal

GREEN = '\033[32m'
RED = '\033[31m'
BOLD = '\033[1m'
RESET = '\033[0m'


def parse_block(block):
    """Parse a single bpftrace output block, return (pmd2, pmd3, file9, total_count, total_time) or None."""
    lines = block.strip().split('\n')
    if not lines:
        return None

    pmd2 = pmd3 = file9 = 0
    total_time = None

    for line in lines:
        line = line.strip()
        m = re.match(r'@scan_pmd_status\[2\]:\s*(\d+)', line)
        if m:
            pmd2 = int(m.group(1))
        m = re.match(r'@scan_pmd_status\[3\]:\s*(\d+)', line)
        if m:
            pmd3 = int(m.group(1))
        m = re.match(r'@scan_file_status\[9\]:\s*(\d+)', line)
        if m:
            file9 = int(m.group(1))
        m = re.match(r'Total time\s*:\s*(\d+)', line)
        if m:
            total_time = int(m.group(1))

    total_count = pmd2 + pmd3 + file9
    if total_count == 0 and total_time is None:
        return None

    return (pmd2, pmd3, file9, total_count, total_time)


def print_results(results):
    """Filter first/last rounds and print statistics."""
    if len(results) > 2:
        results = results[1:-1]
        print(f"Filter out first and lastest round, done.")

    print(f"\n{BOLD}{'Time cost':>10} | {'Before(s)':>10} | {'After(s)':>10} | {'Percent':>8}{RESET}")
    print("-" * 48)

    sum_before = sum_after = sum_reduction = 0
    count = 0

    for idx, (pmd2, pmd3, file9, total_count, total_time) in enumerate(results, 1):
        before_time = total_count / 8 * 10
        after_time = total_time if total_time is not None else 0
        reduction = (after_time - before_time) / before_time * 100 if before_time > 0 else 0

        sum_before += before_time
        sum_after += after_time
        sum_reduction += reduction
        count += 1

        color = GREEN if reduction <= 0 else RED
        print(f"{idx:>10} | {before_time:>10.1f} | {after_time:>10} | {color}{reduction:>+7.1f}%{RESET}")

    if count > 0:
        avg_before = sum_before / count
        avg_after = sum_after / count
        avg_reduction = sum_reduction / count
        color = GREEN if avg_reduction <= 0 else RED
        print(f"{'Average':>10} | {avg_before:>10.1f} | {avg_after:>10.1f} | {color}{avg_reduction:>+7.1f}%{RESET}")

    print()


def run_bpftrace(bt_script, rounds):
    """Run bpftrace, collect N rounds of output, then stop."""
    print(f"Running: sudo bpftrace {bt_script}")
    print(f"Waiting for {rounds} rounds of full scan...\n")

    proc = subprocess.Popen(
        ['sudo', 'bpftrace', bt_script],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, bufsize=1,
    )

    results = []
    current_block = []
    completed = 0

    try:
        print(f"---------- Round 1/{rounds} ----------")
        for line in proc.stdout:
            sys.stdout.write(line)
            sys.stdout.flush()
            current_block.append(line)

            if re.match(r'Total time\s*:', line.strip()):
                parsed = parse_block(''.join(current_block))
                if parsed:
                    results.append(parsed)
                    completed += 1
                current_block = []

                if completed >= rounds:
                    print(f"\nCollected {rounds} rounds, stopping bpftrace...")
                    proc.send_signal(signal.SIGINT)
                    break
                else:
                    print(f"---------- Round {completed + 1}/{rounds} ----------\n")
    except KeyboardInterrupt:
        proc.send_signal(signal.SIGINT)

    proc.wait()
    return results


def parse_log(filepath):
    """Parse from a log file (offline mode)."""
    with open(filepath, 'r') as f:
        content = f.read()

    # Split into rounds by accumulating lines until "Total time",
    # since bpftrace print() inserts blank lines between maps
    # which breaks naive blank-line splitting.
    blocks = []
    current = []
    for line in content.splitlines():
        current.append(line)
        if re.match(r'Total time\s*:', line.strip()):
            blocks.append('\n'.join(current))
            current = []
    if current:
        blocks.append('\n'.join(current))
    results = []
    for block in blocks:
        parsed = parse_block(block)
        if parsed:
            results.append(parsed)

    print_results(results)


def main():
    import argparse
    parser = argparse.ArgumentParser(description='khugepaged scan performance analyzer')
    parser.add_argument('-f', '--file', metavar='LOG_FILE',
                        help='Parse log file (offline mode)')
    parser.add_argument('-b', '--bpftrace', metavar='SCRIPT', nargs='?',
                        const='./khugepaged_mm.bt',
                        help='Run bpftrace script (default: ./khugepaged_mm.bt)')
    parser.add_argument('-r', '--rounds', type=int, default=5,
                        help='Number of rounds to collect (default: 5)')
    args = parser.parse_args()

    if args.file:
        parse_log(args.file)
    elif args.bpftrace:
        results = run_bpftrace(args.bpftrace, args.rounds)
        print_results(results)
    else:
        # Default: run bpftrace with default script and 5 rounds
        results = run_bpftrace('./khugepaged_mm.bt', 5)
        print_results(results)


if __name__ == '__main__':
    main()
