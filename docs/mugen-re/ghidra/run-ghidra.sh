#!/bin/bash
# One-shot headless Ghidra analysis + anchor decompile of mugen.exe.
# Run from the anifight repo root AFTER the standalone Ghidra zip + JDK21 are extracted.
set -euo pipefail

EXE=/tmp/mugen_peek/mugen/mugen.exe
PROJDIR=/tmp/mugen_peek/ghidra-proj
PROJ=mugen10
SCRIPTDIR="$(cd "$(dirname "$0")" && pwd)"
# The postScript writes docs/mugen-re/decompiled relative to the CWD of
# analyzeHeadless: anchor to the repo root no matter where this was launched from.
cd "$SCRIPTDIR/../../.."

AHBIN=/tmp/mugen_peek/ghidra_12.1.4_PUBLIC/support/analyzeHeadless
[ -x "$AHBIN" ] || AHBIN=$(find /tmp/mugen_peek -maxdepth 3 -name analyzeHeadless | head -1)
export JAVA_HOME=/tmp/mugen_peek/jdk21/Contents/Home
export PATH="$JAVA_HOME/bin:$PATH"

echo "analyzeHeadless: $AHBIN"; java -version 2>&1 | head -1

rm -rf "$PROJDIR"; mkdir -p "$PROJDIR"
"$AHBIN" "$PROJDIR" "$PROJ" -import "$EXE" -scriptPath "$SCRIPTDIR" \
  -postScript AnalyzeAnchors.java -max-cpu 8 2>&1 | tail -40

echo "--- decompiled output ---"
ls -la docs/mugen-re/decompiled/ 2>/dev/null
