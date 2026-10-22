# Next steps (prioritised)

Read `PROJECT_STATE.md` first. Environment notes: Google download hosts are blocked in the
agent container; GitHub, Ubuntu archives, PyPI and npm work. Toolchain locations are
listed in `docs/BUILD_ANDROID.md`. Use `ccache` for rebuilds (installed), and avoid editing
widely-included headers (`ewidget.h`, `eresolution.h`) mid-build: a header change costs a
full ~45 min rebuild on 2 cores.

## 0. Status after phone feedback (2026-10-22, Samsung Galaxy S25)
0.1.4 confirmed by the user (zoom, lists, speed label). 0.1.5 adds two-step placement
(preview, drag to nudge, ✓/✕; hold+drag area previews) and the Touch controls screen.
Ask the user to check: placement feel (preview offset, panel position, ✓/✕ size), the
settings screen, a save → kill app → load round trip, pause/resume (home button).

## 1. First on-device run  ← partly done (runs on the user's phone)
* Install `dist/eZeusPort-debug-arm64-v8a.apk` on an arm64 phone (Android 8+).
* Import the game folder + `eZeus-0.8.2-beta.zip`; start; capture `adb logcat -s SDL SDL/APP eZeus ZeusImport`.
* Pass: main menu renders, leader can be created with the soft keyboard, *Zeus and Europa*
  starts, map pans/zooms, a building can be placed, the game saves and reloads after the
  app is killed.
* Record device model, Android version, FPS (enable `showFPS` or add an overlay), RSS
  (`adb shell dumpsys meminfo org.zeusport.game`) in `docs/TEST_REPORT.md`.
* Known risks to look for: GPU memory (interface+i30 decode to ~945 MB RGBA if everything is
  loaded; i15 adds ~210 MB), GL context loss on resume (render targets are rebuilt on
  `SDL_RENDER_TARGETS_RESET`, untested on device), notch/cutout layout, text input.

## 2. Desktop re-verification after the latest engine changes (DONE 2026-10-22, see TEST_REPORT; keep as regression)
* `ninja -C build/desktop` (full rebuild because of header changes), then
  `tools/tests/run_desktop_autotest.sh build/run/touch1.txt build/run/touch1.log EZEUS_TOUCH=1`
  and the same with `EZEUS_RENDER_FPS=60 EZEUS_UI_SCALE=large` in a 2400×1080 Xvfb screen
  (edit the `-screen` in `run_desktop_autotest.sh`) to check the large phone UI layout and the
  touch bar.
* Measure simulation rate: log `mTime`/date at t and t+60 s with render FPS 20 and 60; they
  must match (pass criterion: identical game days advanced ±1 tick).

## 3. Save/load round trip
* Autotest: start Zeus and Europa, place buildings, Escape → save, quit, restart, load,
  compare `debugCameraState` + population/date. Add a `save`/`load` autotest command if the
  menu-click path is too brittle (files: `engine/platform/eautotest.cpp`, `engine/emainwindow.cpp`
  `saveGame/loadGame`).
* Atomic saves are implemented (`*.tmp` → rename, `*.bak`); verify with a kill-during-save test.

## 4. Touch UX completion
* Two-step placement: DONE (0.1.5, default for all tools). Possible follow-ups: show the cost
  on the ✓ button; resize an area preview from either corner; "repeat" (keep a preview next
  to the last building).
* Offset placement cursor so the finger does not hide the footprint.
* Larger list rows / drag-scrolling: DONE for adventure, leader and save/load lists (0.1.4);
  other scroll lists (editor, military, text scrollers) still mouse-sized.
* Settings screen for touch: DONE (0.1.5, `widgets/etouchsettingswidget.cpp`, stored in
  port_settings.txt; Back → Touch controls, or Options → Touch controls).
* Optional start zoom > 1 on phones (e.g. 1.5×) once the user has tried 0.1.4 zoom.
* Kinetic (fling) scrolling for lists and the map.
* Box-select of soldiers (left-drag in "none" mode) has no touch equivalent: proposal =
  long-press then drag.
* Optional offset cursor for precise placement (`eTouchGestures` + `eGameWidget` hover).
* QHD phones: consider `SDL_RenderSetLogicalSize` to keep UI ≥ 7 mm.

## 5. Parity work (Phase F)
* Run the original game on the user's PC (screen-control tools, with permission) for
  side-by-side checks; start with the Olympus fork's claimed fixes (settler group size,
  house devolution, god blessings) and log results in `docs/PARITY_MATRIX.md`.
* Install Ghidra (GitHub release + JDK 21) only when a specific question needs it.

## 6. Release hygiene
* Rebuild with an official NDK/SDK (Gradle path) before any distribution; verify
  16 KB page alignment (`llvm-readelf -l libmain.so`, LOAD align 0x4000).
* Release signing config outside git.
