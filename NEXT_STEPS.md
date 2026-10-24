# Next steps (prioritised)

Read `PROJECT_STATE.md` first. Environment notes: Google download hosts are blocked in the
agent container; GitHub, Ubuntu archives, PyPI and npm work. Toolchain locations are
listed in `docs/BUILD_ANDROID.md`. Use `ccache` for rebuilds (installed), and avoid editing
widely-included headers (`ewidget.h`, `eresolution.h`) mid-build: a header change costs a
full ~45 min rebuild on 2 cores.

## Roadmap: phases and what is left (2026-10-23)

Phases from the brief (§17). ✅ done with evidence, 🟡 partly done, ⬜ not started.

* **A – Reconnaissance and baseline ✅** engine audit, data inventory, desktop build.
* **B – Android-native proof ✅** arm64 APK, SDL glue, logcat, build script; runs on the
  user's Samsung Galaxy S25.
* **C – Original assets ✅** importer, ENG→XML, eZeus pack download, real menus, maps,
  music and sound on the phone (user, 2026-10-23).
* **D – Gameplay and persistence ✅ (long session open)** on the phone: simulation, building,
  save → kill app → load, home button / screen off and back (user, 2026-10-23). Open: a
  longer session (an hour, memory), autosave check, save/load autotest on desktop.
* **E – Touch excellence 🟡** done: gesture FSM, pan, continuous focal pinch zoom, preview
  placement with ✓/✕ and repeat, lists, touch bar, settings screen, 60 FPS, army select /
  move / box select (0.1.7), Draw mode for rows/areas, stats overlay, tap-outside closes
  info, tablet layout check at 2560×1600 (0.1.8). Open: (1) FPS/memory numbers from the
  phone (stats overlay), (2) remaining small lists (editor,
  military, long texts) and info-panel buttons, (3) tablet layout check, (4) FPS/frame-time
  and memory measurements on the phone, (5) optional: fling scrolling, start zoom > 1,
  haptic feedback on ✓. Done in 0.1.12: smart straight roads, Save/Load buttons
  with quick save, autosave on going to the background. Open: in-game FPS 56 (11 ms) on the
  S25 – profile the map draw if it drops further.
* **F – Gameplay parity with the original 🟡 (started 2026-10-24)** done: data parity against
  the original Model files (costs, employees, desirability, tax table; tax low/very-low fix),
  Olympus blessing claim checked and rejected; side-by-side in the original: hut without food,
  settlers in groups of 4 (0.1.10; a road check for houses was removed again in 0.1.11); desirability spread and house
  evolution rules from Zeus.exe (R6, 0.1.10). Next (user: houses level up fine in play, no side-by-side wanted): (1) only on reported
  differences, (2) culture score instead of venue count, (3) fire/damage risk model, (4) immigration
  timing, (5) food/storage/distribution and walkers. Original
  plan: compare eZeus with the original game on the
  user's PC (screen control, with permission), most visible systems first: immigration and
  housing, food/storage/distribution, walkers and services, gods/heroes/monsters, military,
  trade, campaign goals. Log each in PARITY_MATRIX, fix the important differences (Ghidra
  only for specific questions), add regression saves.
* **G – Stabilisation and distribution-ready build ⬜** rebuild with the official Google
  NDK/SDK (current toolchain comes from a third-party mirror), verify the Gradle build,
  release signing only with the user's approval, data-import guide, final test report,
  Windows build check, list of remaining limitations. Nothing is published without approval.

## 0. Status after phone feedback (2026-10-23, Samsung Galaxy S25)
0.1.5 confirmed by the user. 0.1.6 adds "next one ready" repeat placement, price on ✓, red
preview kept. Phase D confirmed on the phone (2026-10-23). 0.1.8: Draw mode, stats overlay, tablet check.
Next: phone FPS/memory numbers from the user, remaining small panels, then Phase F parity.

## 1. First on-device run  ← partly done (runs on the user's phone)
* Install `dist/ZeusDroid-debug-arm64-v8a.apk` on an arm64 phone (Android 8+).
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
