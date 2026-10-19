#!/bin/sh
# Builds and runs the host tests for the expression grammar and command
# parser (tools/hosttest/test_expr_cmd.cpp) with the host clang++ -- no
# gradle, no CMake, so it is safe to run while an Android build is in
# flight. expr.cpp, cmd.cpp and core/ini.cpp need nothing beyond the
# standard library (unlike the SFF codec test, this one does not need -lz).
#
# A nonzero exit means at least one assert failed; the FAIL lines name the
# scenario. Asserts marked BUG in the source document intended behaviour the
# engine does not currently have -- their failure is the suite's bug report.
#
# This builds only the expression/command suite. tools/hosttest/run_all_tests.sh
# builds and runs the whole host suite (state machine, .cns reader, the codecs,
# the screenpack readers) with the same host clang++.

set -e

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
CXX=${CXX:-clang++}
BIN="${TMPDIR:-/tmp}/anifight_test_expr_cmd"

"$CXX" -std=c++20 -O1 -Wall -Wextra -I "$ROOT/app/src/main/cpp" -o "$BIN" \
    "$ROOT/tools/hosttest/test_expr_cmd.cpp" \
    "$ROOT/app/src/main/cpp/engine/expr.cpp" \
    "$ROOT/app/src/main/cpp/engine/cmd.cpp" \
    "$ROOT/app/src/main/cpp/core/ini.cpp"

"$BIN"
