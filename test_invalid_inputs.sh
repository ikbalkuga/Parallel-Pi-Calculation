#!/usr/bin/env bash
set -euo pipefail

echo "Testing invalid inputs..."

expect_failure() {
    local description="$1"
    shift

    if "$@" >/dev/null 2>&1; then
        echo "FAIL: $description"
        exit 1
    else
        echo "PASS: $description"
    fi
}

expect_failure "Serial: missing argument" ./pi_serial
expect_failure "Serial: zero iterations" ./pi_serial 0
expect_failure "Serial: non-numeric input" ./pi_serial abc
expect_failure "Serial: negative input" ./pi_serial -5

expect_failure "Parallel: missing arguments" ./pi_parallel
expect_failure "Parallel: zero iterations" ./pi_parallel 0 4
expect_failure "Parallel: zero threads" ./pi_parallel 1000 0
expect_failure "Parallel: non-numeric iterations" ./pi_parallel abc 4
expect_failure "Parallel: non-numeric threads" ./pi_parallel 1000 abc
expect_failure "Parallel: too many threads" ./pi_parallel 1000 1025

echo "All invalid-input tests passed."

