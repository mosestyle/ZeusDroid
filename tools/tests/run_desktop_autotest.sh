#!/usr/bin/env bash
# Runs the desktop engine headless (Xvfb) with an autotest script.
# usage: run_desktop_autotest.sh <script> <log> [extra env...]
set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SCRIPT="$1"; LOG="$2"; shift 2
export EZEUS_BIN_DIR="${EZEUS_BIN_DIR:-$ROOT/local-assets/ZeusPoseidon/eZeus/Bin/}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"
export EZEUS_AUTOTEST="$SCRIPT"
export EZEUS_AUTOTEST_LOG="${LOG%.log}.autotest.txt"
for kv in "$@"; do export "$kv"; done
timeout "${TIMEOUT:-300}" xvfb-run -a -s "-screen 0 1920x1080x24" "$ROOT/build/desktop/eZeus" > "$LOG" 2>&1
rc=$?
cat "${LOG%.log}.autotest.txt" 2>/dev/null
echo "exit=$rc"
