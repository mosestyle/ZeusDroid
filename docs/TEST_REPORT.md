# Test report

Environment for all results below: Ubuntu 24.04 container, x86-64, 2 vCPU, no GPU
(Xvfb + software rendering), no Android device or emulator. Game data: the user's GOG
Zeus + Poseidon (English, Zeus.exe v1.11) staged read-only from the linked PC, plus the
official eZeus 0.8.2-beta release zip. Screenshots are kept locally in `build/run/` and are
not committed (they show copyrighted game art).

## 1. Unit tests – `tools/tests/run_unit_tests.sh`

| Suite | Host | Result |
|---|---|---|
| Touch gesture FSM (`tests/native/test_touch_gestures.cpp`, 11 scenarios) | x86-64 GCC 13 | 50 checks, 0 failures |
| Same, built as static `aarch64-linux-android26` binary | qemu-aarch64 | 50 checks, 0 failures |
| ENG→XML (`tests/test_eng_to_xml.py`): synthetic text+message files, escaping, cp1252, garbage rejection; native vs Python | x86-64 | OK |
| ENG→XML on the user's `Zeus_Text.eng` / `Zeus_MM.eng`: Python == native C++ (x86-64) == native C++ (ARM64 under qemu) | both | byte-identical (sha256 `577ec188…`, `cb5e23ef…`) |
| Importer rules (`tests/java/ImportRulesTest.java`): traversal, zip-slip, mapping, skips, validation | OpenJDK 21 | 36 checks, 0 failures |
| Data validation (`tools/assets/validate_game_data.py check`) on staged data | x86-64 | 0 required files missing (audio was not staged: 1549 optional missing) |

## 2. Desktop engine with real data (headless autotests)

Build: `cmake -S engine -B build/desktop -G Ninja -DCMAKE_BUILD_TYPE=Release -DEZEUS_SYSTEM_SDL=ON && ninja` → exit 0 (GCC 13.3, SDL2 2.30 system packages).
Run: `tools/tests/run_desktop_autotest.sh <script> <log> [EZEUS_TOUCH=1]`, 1280×720 window.

| Script | Observed (log / screenshot) | Result |
|---|---|---|
| menu | Roster of Leaders over the original Zeus registry background | pass |
| create_leader | "Enter leader name" dialog, typed "Moses", leader saved to `eZeus/Save/moses` | pass |
| explore4 | Main menu → Adventures list containing Zeus and Poseidon adventures | pass |
| explore5 | *Zeus and Europa* → "Founding of Thebes" briefing, goal "Population of 1000" | pass |
| start_zeus_europa | `wait_game ok t≈20.7 s`; city view of Thebes, top bar (5000 dr, 1 Jan), side panel, "Game paused" | pass |
| touch_gestures_zeus_europa (`EZEUS_TOUCH=1`) | one-finger pan: camera `dx 0→-300, dy -15→-165` (exactly the finger delta); pinch-in at (500,400): tile 60×30 → 30×15; jitter tap: camera unchanged; game unpaused: date 1 → 2 Jan, "Hydra in city" event; second pinch started on the message box and was correctly treated as a UI touch | pass (zoom-in step not exercised because of the modal message; covered by unit tests) |

Not yet re-run after the later engine changes (frame clock, touch bar, UI-scale override,
atomic saves, tooltip suppression) – see NEXT_STEPS §2.

## 3. ARM64 codec check

`tests/native/test_minimp3_decode.c` compiled for `aarch64-linux-android26` with the exact
minimp3 sources SDL_mixer 2.8.2 uses on Android, run under qemu on the user's music:
all 26 `Audio/Music/*.mp3` files decoded completely (44.1 kHz stereo, 3.5 s – 312 s) and an
adventure voice file (`Adventures/The Odyssey/The Odyssey_A_v.mp3`, 44.1 kHz mono, 87.8 s);
exit 0. WAV files go through SDL_mixer's built-in WAV loader (not separately tested).

## 4. Android build (2026-10-22)

Command: `tools/build_android.sh` (exit 0). Toolchain: host clang-20/lld-20 with NDK r29
sysroot (third-party mirror, see BUILD_ANDROID.md), Debian aapt2 2.19, d8 9.2.4-dev,
Debian apksigner 0.9.

| Check | Result |
|---|---|
| Native build | 5 shared libraries, all `ELF 64-bit LSB shared object, ARM aarch64` |
| `libmain.so` | 15.9 MB stripped; exports `SDL_main`; NEEDED only libSDL2*.so + libandroid/liblog/libm/libdl/libc |
| `libSDL2.so` | 52 `Java_org_libsdl_app_*` JNI exports; NEEDED libGLESv2/libGLESv1_CM/libOpenSLES/libandroid/liblog (all NDK system libs) |
| C++ runtime | static libc++ (no libc++_shared.so dependency) |
| Page alignment | every LOAD segment aligned 0x4000 (16 KB-page devices OK) |
| Manifest | package org.zeusport.game, versionCode 1, minSdk 26, targetSdk 35, native-code arm64-v8a, launcher ImportActivity |
| dex | ImportActivity, ImportRules, ZeusActivity, org.libsdl.app.* present |
| Assets | 9 licence files under assets/licenses |
| zipalign -c -p 4 | OK |
| apksigner verify | v2 + v3 signatures valid (debug key) |
| Artifact | `dist/eZeusPort-debug-arm64-v8a.apk`, 18 892 466 bytes, sha256 `13eba1f09b864b9fcf1233e659e34f73799d7d543bfe75541c154818b92a31c4` |
| Emulation layers | none (no x86 code, no Box64/Wine) |

**Not tested:** installation and launch on a device or emulator (none available; Google
emulator images cannot be downloaded here, and the container has no KVM). Everything
about runtime behaviour on Android is therefore unverified.

## 5. Results appended during the session

### 0.1.1-dev (2026-10-22, after first on-device report)

First real-device report from the user (phone, Android, screenshot): APK **installed and
the importer ran** – game folder import OK (all game items green, music + voices found).
Importing GitHub's *source-code* zip (`archive/refs/tags/0.8.2-beta.4.zip`) left
`interface.e` and `i15/i30.e` missing, because that archive does not contain them.
Fixes: whitelist of pack files, explicit "this is the source code zip" error, clean-up of
source files copied by 0.1.0, and an in-app download of the official release pack with
resume + size + SHA-256 check (redirect chain and HTTP Range verified with curl).
Importer tests: 46 checks, 0 failures. APK `0.1.1-dev` (versionCode 2), 18 900 658 bytes,
sha256 `b383481bc733ebbfa97e9db8e8f942bf6117ec78ac231831c080f6f9224dbd33`, same debug key
(installs over 0.1.0). Game start on device: still unverified.

### Desktop re-verification of the later engine changes (2026-10-22)

Desktop rebuilt (702/702, exit 0) with the frame clock, touch bar, UI-scale override, atomic
saves and tooltip changes.

**Simulation rate vs render rate** (`tests/autotest/simrate_long.txt`, 20 s window after
dismissing the start-of-game messages, game speed 10):

| `EZEUS_RENDER_FPS` | sim units in 20 s | ticks/s |
|---|---|---|
| 20 (upstream loop) | 3970 | 19.84 |
| 60 (Android default) | 4000 | 19.99 |

The Hydra event fired at the same simulation time (360) in both modes.

**Phone layout** (Xvfb 2400×1080, `EZEUS_UI_SCALE=large`, `EZEUS_TOUCH=1`, 60 FPS, finger
events only): main menu → New adventure → *Zeus and Europa* → briefing → city all reached by
taps. Touch bar: `||` unpaused (paused 1→0), `+` raised speed 10→25. Pinch-in 60×30→30×15;
pinch-out back to 60×30 anchored exactly at the centroid (dx 0→-1000, dy -7→-614 for
centre (1000,600), ×2). Long-press on empty map changed nothing. RSS at game start 504 MB
(software renderer, textures in system RAM).

**Bug found and fixed:** the four touch-bar buttons counted as "open dialogs" in eZeus'
`children().size() < 6` check, so the simulation never advanced with the touch bar
(simtime stayed 0). Excluded via `mTouchBarButtons`; re-test: simtime advanced 0→375 until
the original Hydra message box (which pauses the game in vanilla too).

**UX issue noted:** adventure-list rows are ~26 px tall at 2400×1080 – too small for
reliable finger selection (NEXT_STEPS).

### APK 0.1.2-dev (2026-10-22)
Rebuilt with the touch-bar simulation fix (31 objects recompiled, `egamewidgetpaint.cpp.o`
newer than the source change). versionCode 3, 18 900 658 bytes, sha256
`fdc32a06f0992eef9174144277cac2ca26c57402d04681e11cb0d6408bd8f58d`, v2/v3 signature OK,
same debug key (installs over 0.1.0/0.1.1). On-device game start: still unverified.

### First on-device run (user report, 2026-10-22 13:23)

The user's phone (model/Android version not reported) ran the game: city view of an Athens
adventure at 60 FPS with the large UI and the touch bar (screenshot from the user). Data
import with the in-app eZeus pack download worked. Reported: `-`, `+`, `R` had no visible
effect and the date stayed at 1 Jan 1500 BC – consistent with 0.1.1 (frozen-simulation bug
fixed in 0.1.2); the reproduction on desktop with *The Founding of Athens* in 0.1.2 advanced
to 4 Jan with working speed buttons. Device version to be confirmed (now shown on the import
screen). Feedback: one-finger drag in build mode should pan → implemented below.

### 0.1.3-dev: hold-to-draw build controls (desktop, phone layout, finger events)

`tests/autotest/phone_build_hold_to_draw.txt`: road icon tap → build tool active and X shown;
quick one-finger drag in build mode → camera dx 0→-200, dy -15→-95, nothing built
(5000 dr); hold 450 ms then drag (1300,600)→(1600,750) → road preview 11×1 during drag,
road built on release; X → build tool off. (A first attempt started on rocky terrain where
eZeus cannot build roads and correctly built one tile only.) Unit tests: 60 checks, 0
failures (x86-64 and ARM64/qemu), incl. new hold-to-draw scenario.
APK 0.1.3-dev: versionCode 4, sha256 5d978c470cbefc3610f431364868a346b9cfcc1ccc1a329b7c01d8f2619d63b1, v2/v3 OK

### Second on-device report (user, 2026-10-22 22:42) – Samsung Galaxy S25
0.1.3 on the phone: date advances, speed works. Problems: "Speed Nx" tips stack (up to 5)
and hide the current speed; pinch zoom asked for. → 0.1.4 below.

### 0.1.4-dev: continuous zoom, speed label, touch lists (desktop, phone layout, finger events)
Xvfb 2400×1080, `EZEUS_TOUCH=1 EZEUS_UI_SCALE=large EZEUS_RENDER_FPS=60`, real game data.
* Unit tests: 74 checks, 0 failures (x86-64 and ARM64/qemu); importer rules 46/46.
* `tests/autotest/phone_zoom_speed_lists.txt`: adventure list rows ≈ 84 px; one-finger drag
  scrolled the list by the drag distance without selecting; drag back restored; tap selected
  *The Founding of Athens*. In the city: tap at P=(1000,600) → hover tile 37,3; slow pinch
  around P (spread 200→400) → zoom 2.0, dx 0→-500, dy -15→-315 (the world point under P is
  unchanged: 1000/2+500 = 1000, 600/2+315 = 615), tap at P → still tile 37,3; two-finger pan
  (-60,-60) px → dx/dy -30/-30 world px, zoom stays 2.0 (dead band); pinch in → zoom 0.5
  (i15, scale 1). `+` twice → speedId 3, label "5x" (screenshot).
* `tests/autotest/phone_build_zoom2.txt` (hold-to-draw at zoom 2): quick drag of 200 px
  panned 100 world px (= 200 screen px), hold + drag (1300,600)→(1600,750) drew a 6-tile road
  from the hold point to the finger (half the 11 tiles at zoom 1), X left build mode.
* Regressions: `phone_build_hold_to_draw.txt` (zoom 1: pan -200, 11-tile road, X) and
  `phone_touch_bar_pinch.txt` (pause → simulation runs, `+`, pinch 0.6×/1.2×, simtime
  advancing) pass with the new button positions; desktop 1280×720 mouse
  `start_zeus_europa.txt` unchanged (zoom 1, no off-screen pass).
* Visual check of a zoom-2 screenshot: linear upscaling, no tile seams.

### Third on-device report (user, 2026-10-22 23:15)
0.1.4 on the S25: "everything works great so far" (zoom, speed label, lists). Request: placing
buildings by tap is imprecise; ideas for placing several buildings. → 0.1.5 below.

### 0.1.5-dev: two-step placement, touch settings screen (desktop, phone layout, finger events)
* Unit tests: 88 checks, 0 failures (x86-64 and ARM64/qemu), incl. new confirm-placement
  scenario (tap previews, drag on preview moves it, drag elsewhere pans, hold+drag previews
  an area, no press/release ever sent, lifecycle cancel safe); importer rules 46/46.
* `tests/autotest/phone_build_confirm.txt` (road tool, zoom 1): tap → preview 42,-2, panel
  shown, 10000 dr unchanged; drag on preview +115 px → preview 44,-4, camera unchanged;
  drag elsewhere → dx -100, preview stays on 44,-4, panel follows; ✓ → 9995 dr (one tile),
  preview gone, tool still active; hold+drag (1100,500)→(1400,650) → area preview
  37,-3 → 47,-3, nothing built; ✕ → preview gone, 9995 dr; same area + ✓ → 9940 dr
  (11 tiles); X → tool off. Screenshots checked (green preview, ✓/✕ panel, road built).
* `tests/autotest/phone_back_touch_settings.txt`: Back after tapping "+" opens the game menu
  (with "Touch controls"); settings screen renders 8 rows; switching "Placing buildings" to
  Instant wrote `build_confirm "off"` to port_settings.txt and took effect without restart
  (tap with the road tool built at once: 10000 → 9995 dr).
* `tests/autotest/phone_options_touch_settings.txt`: main menu Options shows "Touch controls";
  resolution / full-screen buttons hidden on touch.
* Regressions: instant mode (`EZEUS_BUILD_CONFIRM=0`) `phone_build_hold_to_draw` (pan -200,
  11-tile road, X) and `phone_build_zoom2` (6-tile road at zoom 2); `phone_zoom_speed_lists`,
  `phone_touch_bar_pinch` unchanged results.
* Fixed test tooling: autotest `key` joins arguments, so names with spaces ("AC Back") work
  (an earlier Back check had silently sent no key).
APK 0.1.5-dev: versionCode 6, sha256 28391ce090829936e6c26df0dd57e8f9be0342ce32016c2e93e89dc250fbe70a, v2/v3 OK

### Fourth on-device report (user, 2026-10-22 23:56)
0.1.5 on the S25: "works great" (preview placement, settings screen).

### 0.1.6-dev: "next one ready" repeat placement, price on ✓, red preview kept
* `tests/autotest/phone_build_repeat.txt` (wheat farm, 3×3, zoom 1): farm tool via
  Husbandry → popup "Wheat Farm 45"; tap → green preview 27,8, ✓ shows "45"; ✓ → 9955 dr,
  next preview appears at 30,8 (one farm along +x); dragged along the coast to 30,5;
  ✓ → 9910 dr, next preview at 33,2 (same spacing as the last two); ✓ → 9865 dr, next
  preview 36,-1. Screenshot: three farms in a row plus the green fourth preview.
  Earlier run: ✓ on a red preview (dry land) built nothing and the preview stayed.
* `phone_build_confirm.txt`: after ✓ on a road tile the next preview appears beside it; area
  preview, ✕, area ✓ (11 tiles, 9940 dr) and X unchanged.
* Unit tests 88/88 (x86-64, ARM64/qemu); importer 46/46.
APK 0.1.6-dev: versionCode 7, sha256 4f4afeb087f6e0adc0677b671201483310807eb20534249c825d9cae43563b0f, v2/v3 OK

### Phase D on the device (user report, 2026-10-23 00:15, Samsung Galaxy S25, 0.1.6)
* Music and sound effects play.
* Save → swipe the app away → reopen → load: works.
* Home screen and screen off/on in the middle of a game, then back: everything works
  (no lost textures, audio resumes).

### 0.1.7-dev: touch army control
* Unit tests 94/94 (x86-64, ARM64/qemu): new box-select scenario (drag = left-drag from the
  touch-down point, no pan, no long press; tap still a tap); importer 46/46.
* `tests/autotest/phone_army.txt` (banners created with the test-only `debug_banner`
  command, which adds a player banner without soldiers): "[ ]" button appears when the city
  has banners; tap on a banner → 1 selected; tap on the map → banner moved 37,3 → 38,-5,
  still selected; X → 0 selected; "[ ]" → box mode, drag (950,430)→(1450,760) → selection
  rectangle drawn (screenshot), both banners selected, box mode off again; next drag pans
  (dx -100), selection kept. Real soldiers walking to the banner not covered by this test
  (eZeus moves soldiers to their banner; unchanged code).
* Regressions after the touch-bar change (X moved down one slot): phone_build_confirm,
  phone_build_repeat, phone_back_touch_settings (row positions updated for the new
  "After building" row), instant-mode phone_build_hold_to_draw / phone_build_zoom2: all pass.
APK 0.1.7-dev: versionCode 8, sha256 8b225df8967a70a72764bf2f24970f6ef29628ca00bd9cb856dd6d2cd7219aee, v2/v3 OK

### User report 2026-10-23 00:35 (0.1.7)
Army controls fine. Hold-then-drag for rows/areas works only sometimes. Likely causes: the
finger moving more than the tap slop (24 px ≈ 1.5 mm on the S25) during the 0.3 s hold turns
the gesture into a pan, and a hold that starts within 2 tiles of the "next one ready"
preview is treated as dragging that preview. → 0.1.8 "Draw" mode.

### 0.1.8-dev: Draw mode, stats overlay, tablet check, info windows
* Unit tests 100/100 (x86-64, ARM64/qemu): new draw-mode scenario (confirm: drag → area
  preview from the touch-down point without a hold, no pan; two fingers pan; tap = single
  preview; instant: drag → left-drag).
* `phone_build_draw.txt`: road tool starts with Draw on (yellow button in the "[ ]" slot);
  quick drag (1100,500)→(1400,650) → road preview 35,-2→45,-2, nothing built; ✓ → 55 dr
  (11 tiles); two-finger drag → dx -50, no preview; Draw off → one-finger drag pans (dx
  -150); wheat farm → Draw off by default, button shown.
* `phone_stats_overlay.txt`: overlay bottom-left "fps / ms / ticks/s / MB". In this container
  (software rendering at 2400×1080, 2 vCPU) it read 16 fps, 62.7 ms, 16 ticks/s, 522 MB –
  container numbers only; the phone measurement is pending (the user enables it under Touch
  controls → Show FPS and memory).
* `phone_info_window.txt`: long press on a farm → info window; tap on the map outside it →
  closed.
* Tablet layout (desktop, touch mode, UI large) at 2560×1600: menu, adventure list (rows now
  capped at 84 px so tall screens show more rows), briefing, city view with touch bar and the
  taller side panel – screenshots checked, no overlap. 1280×800 (UI small): main menu checked.
* Touch controls screen now sizes its rows to fit the screen (10 rows at 1080 px; it
  overflowed before the fix).
* Regressions (old tests turn Draw off for roads first): confirm placement, repeat, army,
  zoom/lists/speed, touch bar/pinch, instant hold-to-draw at zoom 1 and 2 – all as before.
APK 0.1.8-dev: versionCode 9, sha256 6e86b78d297f31fcddce3cfd3c18eebd94d26abe378a1e78ea798bc628158325, v2/v3 OK

### User report 2026-10-24 11:56 (0.1.8)
Draw for roads and houses works, "could be better". Asked to continue with Phase F.

### 0.1.9-dev: freehand roads, edge auto-scroll, data parity
* Unit tests 102/102 (x86-64, ARM64/qemu): previewArea begin flag, previewEnd on lift and on
  cancel.
* `tests/autotest/phone_build_freehand.txt`: drag right then down → 34-tile road preview
  following the finger with the turn (screenshot; the tile in the water red); ✓ shows "99"
  and builds 33 tiles for 99 dr (Hero, 3 dr/tile from the original Model data); drag to the
  right map edge and hold → dx 0 → -205 → -866 within 1.2 s, the road grows 41 → 69 tiles;
  after lifting the finger the map stops (dx stays -945).
* Parity: `tools/parity/compare_model.py` on the user's Model files: cost 341/540, fire
  158/540, damage 115/540, desirability 10/91, employees 9/54 values differ between eZeus'
  built-in tables and the original (local report build/parity/model_compare.md). After the
  change, in-game road cost on Hero = 3 dr/tile (original) instead of 5; farm 45 (unchanged,
  same in both).
* Tax rate fix checked by code reading (name/rate/sentiment now consistent with the
  original's "Very Low 3 %, Low 7 %" and the finance panel order); no autotest.
* Vibration: SDL haptics on Android (VIBRATE permission present) – not testable here.
* Regressions (with new road cost): confirm, repeat, army, draw, zoom/lists/speed, touch
  bar/pinch, info window, settings, instant hold-to-draw at zoom 1 and 2 – all pass.
APK 0.1.9-dev: versionCode 10, sha256 50a07d2f97c0e71cfaaf6ac012af13794517602d9e2a2d76f3545712828311ce, v2/v3 OK
