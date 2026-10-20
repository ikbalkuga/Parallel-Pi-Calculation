#!/usr/bin/env bash
set -euo pipefail

# Benchmark script for Parallel Pi Number Calculation.
# It uses /usr/bin/time, as suggested in the assignment.
#
# Usage:
#   ./benchmark.sh <iterations>
#
# Example:
#   ./benchmark.sh 1000000000
#
# Choose an iteration count that makes the serial version run for about
# 2-5 seconds on the test machine.

ITERATIONS="${1:-}"
THREAD_COUNTS=(1 2 4 8)
RESULT_FILE="benchmark_results.md"

print_usage() {
    echo "Usage: $0 <iterations>" >&2
    echo "Example: $0 1000000000" >&2
}

require_command() {
    local command_name="$1"
    if ! command -v "$command_name" >/dev/null 2>&1; then
        echo "Error: required command '$command_name' was not found." >&2
        exit 1
    fi
}

validate_iterations() {
    local value="$1"

    if [[ -z "$value" ]]; then
        echo "Error: iterations argument is required." >&2
        print_usage
        exit 1
    fi

    if [[ ! "$value" =~ ^[0-9]+$ ]]; then
        echo "Error: iterations must contain digits only." >&2
        exit 1
    fi

    if [[ "$value" == "0" ]]; then
        echo "Error: iterations must be greater than zero." >&2
        exit 1
    fi
}

measure_program() {
    local mode="$1"
    local threads="$2"
    shift 2

    local output_file
    local error_file
    local time_file
    output_file="$(mktemp)"
    error_file="$(mktemp)"
    time_file="$(mktemp)"

    if ! /usr/bin/time -f "%e" -o "$time_file" "$@" >"$output_file" 2>"$error_file"; then
        echo "Error: benchmark command failed for mode=$mode threads=$threads." >&2
        echo "Command stderr:" >&2
        cat "$error_file" >&2
        rm -f "$output_file" "$error_file" "$time_file"
        exit 1
    fi

    local real_time
    local pi_value
    real_time="$(cat "$time_file")"
    pi_value="$(awk '/^pi:/ {print $2; exit}' "$output_file")"

    if [[ -z "$pi_value" ]]; then
        echo "Error: could not read pi value from program output." >&2
        cat "$output_file" >&2
        rm -f "$output_file" "$error_file" "$time_file"
        exit 1
    fi

    printf '| %s | %s | %s | %s |\n' "$mode" "$threads" "$real_time" "$pi_value" | tee -a "$RESULT_FILE"

    rm -f "$output_file" "$error_file" "$time_file"
}

validate_iterations "$ITERATIONS"
require_command gcc
require_command make

if [[ ! -x /usr/bin/time ]]; then
    echo "Error: /usr/bin/time was not found or is not executable." >&2
    exit 1
fi

make all >/dev/null

{
    echo "# Benchmark Results"
    echo
    echo "Iterations: $ITERATIONS"
    echo
    echo "| Program | Threads | Real time with /usr/bin/time (s) | Computed Pi |"
    echo "|---|---:|---:|---:|"
} > "$RESULT_FILE"

measure_program "serial" "-" ./pi_serial "$ITERATIONS"
for thread_count in "${THREAD_COUNTS[@]}"; do
    measure_program "parallel" "$thread_count" ./pi_parallel "$ITERATIONS" "$thread_count"
done

echo
echo "Benchmark table saved to: $RESULT_FILE"
