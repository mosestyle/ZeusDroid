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
