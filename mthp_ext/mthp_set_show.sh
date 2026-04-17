#!/bin/bash
# mTHP: set all sizes to always and print stats table
#
# Usage: mthp.sh [-e VALUE] [-t] [-s] [-h]
#   -e VALUE  Set all hugepage sizes to VALUE (always/never/madvise/inherit)
#             and print enabled status table
#   -t        Print enabled status table only
#   -s        Print stats table only
#   (default: print both)

SET_ENABLED=""
SHOW_ENABLED=0
SHOW_STATS=0

while getopts "e:tsh" opt; do
	case $opt in
		e) SET_ENABLED="$OPTARG" ;;
		t) SHOW_ENABLED=1 ;;
		s) SHOW_STATS=1 ;;
		h) echo "Usage: $0 [-e VALUE] [-t] [-s] [-h]"
		   echo "  -e VALUE  Set all hugepage sizes to VALUE (always/never/madvise/inherit)"
		   echo "            and print enabled status table"
		   echo "  -t        Print enabled status table only"
		   echo "  -s        Print stats table only"
		   echo "  (default: print both)"
		   exit 0 ;;
		*) exit 1 ;;
	esac
done

# -e implies showing enabled table
if [ -n "$SET_ENABLED" ]; then
	SHOW_ENABLED=1
fi

# Default: if no flag given, show both
if [ -z "$SET_ENABLED" ] && [ $SHOW_ENABLED -eq 0 ] && [ $SHOW_STATS -eq 0 ]; then
	SHOW_ENABLED=1
	SHOW_STATS=1
fi

THP_DIR="/sys/kernel/mm/transparent_hugepage"

# Collect all hugepages-* directories, sorted by size
HUGEPAGES=$(ls -d ${THP_DIR}/hugepages-* 2>/dev/null | sort -t'-' -k2 -h)

if [ -z "$HUGEPAGES" ]; then
	echo "No mTHP sizes found under ${THP_DIR}"
	exit 1
fi

# Step 1: Set all sizes to the specified value (only with -e flag)
if [ -n "$SET_ENABLED" ]; then
	for dir in $HUGEPAGES; do
		[ -f "${dir}/enabled" ] && echo "$SET_ENABLED" > "${dir}/enabled" 2>/dev/null
	done
fi

# Step 2: Print enabled status table
if [ $SHOW_ENABLED -eq 1 ]; then
printf "%-18s %-40s %-40s\n" "Hugepage size" "enabled" "shmem_enabled"
printf "%-18s %-40s %-40s\n" "------------------" "----------------------------------------" "----------------------------------------"
printf "%-18s %-40s %-40s\n" "global" "$(cat ${THP_DIR}/enabled)" "$(cat ${THP_DIR}/shmem_enabled)"
for dir in $HUGEPAGES; do
	size=$(basename "$dir")
	if [ -f "${dir}/enabled" ]; then
		enabled=$(cat "${dir}/enabled")
	else
		enabled="N/A"
	fi
	if [ -f "${dir}/shmem_enabled" ]; then
		shmem=$(cat "${dir}/shmem_enabled")
	else
		shmem="N/A"
	fi
	printf "%-18s %-40s %-40s\n" "$size" "$enabled" "$shmem"
done
echo ""
fi

# Step 3: Print stats table
if [ $SHOW_STATS -eq 0 ]; then
	exit 0
fi
# Collect all unique stat names across all sizes
STATS=""
for dir in $HUGEPAGES; do
	if [ -d "${dir}/stats" ]; then
		STATS=$(echo "$STATS"; ls "${dir}/stats/" 2>/dev/null)
	fi
done
STATS=$(echo "$STATS" | sort -u | sed '/^$/d')

if [ -z "$STATS" ]; then
	echo "No stats found"
	exit 0
fi


# Calculate first column width: max of "mTHP Stats" and all stat names
FIRST_COL="mTHP Stats"
FIRST_W=${#FIRST_COL}
for stat in $STATS; do
	[ ${#stat} -gt $FIRST_W ] && FIRST_W=${#stat}
done

# Calculate column width for each size: max of header and all data values
declare -a COL_WIDTHS
declare -a COL_NAMES
i=0
for dir in $HUGEPAGES; do
	name=$(basename "$dir" | sed 's/hugepages-//')
	COL_NAMES[$i]="$name"
	w=${#name}
	for stat in $STATS; do
		val=$(cat "${dir}/stats/${stat}" 2>/dev/null || echo "N/A")
		[ ${#val} -gt $w ] && w=${#val}
	done
	COL_WIDTHS[$i]=$w
	i=$((i + 1))
done

# Build header
header=$(printf "%-${FIRST_W}s" "$FIRST_COL")
for ((i=0; i<${#COL_NAMES[@]}; i++)); do
	header+=$(printf " %${COL_WIDTHS[$i]}s" "${COL_NAMES[$i]}")
done
echo "$header"

# Separator
sep=$(printf '%*s' "$FIRST_W" '' | tr ' ' '-')
for ((i=0; i<${#COL_WIDTHS[@]}; i++)); do
	dash=$(printf '%*s' "${COL_WIDTHS[$i]}" '' | tr ' ' '-')
	sep+=$(printf " %s" "$dash")
done
echo "$sep"

# Print each stat row
for stat in $STATS; do
	row=$(printf "%-${FIRST_W}s" "$stat")
	i=0
	for dir in $HUGEPAGES; do
		val=$(cat "${dir}/stats/${stat}" 2>/dev/null || echo "N/A")
		row+=$(printf " %${COL_WIDTHS[$i]}s" "$val")
		i=$((i + 1))
	done
	echo "$row"
done
