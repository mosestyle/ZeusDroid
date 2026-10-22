#!/usr/bin/env bash
# Imports + auto-analyses the user's Zeus.exe into a local Ghidra project (once),
# then runs a project-owned script. Projects/outputs stay outside git.
# usage: run_headless.sh /path/to/Zeus.exe <Script.java> [script args...]
set -euo pipefail
GHIDRA="${GHIDRA_HOME:-/home/claude/tools-ghidra/ghidra_12.1.4_PUBLIC}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PROJ="${GHIDRA_PROJECT_DIR:-$ROOT/build/ghidra}"
EXE="$1"; SCRIPT="$2"; shift 2
mkdir -p "$PROJ"
if [ ! -d "$PROJ/zeus.rep" ]; then
  "$GHIDRA/support/analyzeHeadless" "$PROJ" zeus -import "$EXE" -overwrite \
    -max-cpu 2 2>&1 | tail -5
fi
"$GHIDRA/support/analyzeHeadless" "$PROJ" zeus -process "$(basename "$EXE")" -noanalysis \
  -scriptPath "$ROOT/tools/ghidra" -postScript "$SCRIPT" "$@" 2>&1 | grep -v "^INFO" | tail -5
