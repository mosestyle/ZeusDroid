#!/usr/bin/env bash
# Runs host-side unit tests: native (gesture FSM, ENG converter) and Java (importer rules).
# If an Android NDK is available, the native tests are also built as static
# arm64 Android executables and run under qemu-aarch64 when present.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
B="$ROOT/build/tests"; mkdir -p "$B"
fail=0
echo "== native (host)"
g++ -std=c++17 -Wall -Wextra -O1 -I"$ROOT/engine" "$ROOT/tests/native/test_touch_gestures.cpp" \
    "$ROOT/engine/platform/etouchgestures.cpp" -o "$B/test_touch_gestures"
"$B/test_touch_gestures" | tail -1 || fail=1
g++ -std=c++17 -Wall -Wextra -O1 -I"$ROOT/engine" $(sdl2-config --cflags 2>/dev/null) \
    "$ROOT/tests/native/test_desirability.cpp" "$ROOT/engine/engine/boardData/eheatmap.cpp" \
    -o "$B/test_desirability"
"$B/test_desirability" | tail -1 || fail=1
g++ -std=c++17 -Wall -O1 -I"$ROOT/engine" "$ROOT/tests/native/test_qoi_resample.cpp" \
    "$ROOT/engine/texbuild/eqoi.cpp" "$ROOT/engine/texbuild/eresample.cpp" -o "$B/test_qoi_resample"
"$B/test_qoi_resample" | tail -1 || fail=1
g++ -std=c++17 -O2 -I"$ROOT/engine" "$ROOT/tests/native/eng_convert_cli.cpp" \
    "$ROOT/engine/platform/eengconverter.cpp" -o "$B/eng_convert_cli"
python3 "$ROOT/tests/test_eng_to_xml.py" "$B/eng_convert_cli" || fail=1
NDK="${ANDROID_NDK:-/home/claude/android/android-ndk-r29}"
TB="$NDK/toolchains/llvm/prebuilt/linux-x86_64"
if [ -x "$TB/bin/clang++" ] && command -v qemu-aarch64 >/dev/null; then
  echo "== native (arm64 Android, static, qemu)"
  "$TB/bin/clang++" --target=aarch64-linux-android26 --sysroot="$TB/sysroot" -std=c++17 -static -O2 \
    -I"$ROOT/engine" "$ROOT/tests/native/test_touch_gestures.cpp" "$ROOT/engine/platform/etouchgestures.cpp" \
    -o "$B/test_touch_gestures.arm64"
  qemu-aarch64 "$B/test_touch_gestures.arm64" | tail -1 || fail=1
fi
echo "== java (importer rules)"
mkdir -p "$B/java"
javac -nowarn -d "$B/java" "$ROOT/android/app/src/main/java/org/zeusport/game/ImportRules.java" \
    "$ROOT/tests/java/ImportRulesTest.java" 2>&1 | grep -v JAVA_TOOL || true
java -cp "$B/java" ImportRulesTest 2>&1 | grep -v JAVA_TOOL | tail -1 || fail=1
exit $fail
