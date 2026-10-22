# Project state

_Session 1 – 2026-10-22 (agent: Claude, cloud container linked to the user's PC for file access)._
_Branch `main`; see `git log` for the current commit._

## Environment

* Container: Ubuntu 24.04 x86-64, 2 vCPU, 7 GB RAM. No KVM, no Android device/emulator.
* Network: GitHub, Ubuntu archive, PyPI, npm reachable; **dl.google.com, maven.google.com,
  services.gradle.org, Maven Central blocked** (policy, HTTP 403).
* User PC: `C:\GOG Games\Zeus and Poseidon` reachable read-only through the device bridge
  (file listing/staging only; no shell on the PC). 497 files were staged into the container
  (DATA, Model, Adventures, .eng, Zeus.exe, a few config files); Audio was not staged.
* Toolchain: CMake 3.28, Ninja, GCC 13.3 (desktop), clang-20/lld-20 (Android, wrapped),
  NDK r29 sysroot + android-35 jar + d8 from a third-party GitHub mirror (unverified
  provenance), Debian aapt2/zipalign/apksigner, OpenJDK 21, qemu-aarch64, ccache.

## What builds / runs (evidence in docs/TEST_REPORT.md)

| Item | Status |
|---|---|
| Desktop Linux build of eZeus + port layer | builds; runs headless with the user's real data |
| Real game content on desktop | menus, roster, adventure list (Zeus + Poseidon), briefing, Thebes map, running simulation, Hydra event – screenshots in `build/run/` (local only) |
| Touch layer in real engine (desktop, EZEUS_TOUCH=1) | pan exact, continuous pinch zoom 0.5–3× pinned under the fingers, hold-to-draw at zoom 1 and 2, two-step placement (preview, move, ✓/✕), touch settings screen, list drag-scroll, speed label, jitter tap ignored, UI-vs-map routing |
| Unit tests | gesture FSM 94/94 (x86-64 and ARM64-Android-under-qemu), ENG converter (synthetic + real files, Python == C++), importer rules 46/46 |
| Android arm64 native build | builds: libmain.so + SDL2 family, AArch64, 16 KB-aligned, static libc++ |
| APK | `dist/eZeusPort-debug-arm64-v8a.apk` 0.1.7-dev (versionCode 8, sha256 `8b225df8967a70a72764bf2f24970f6ef29628ca00bd9cb856dd6d2cd7219aee`) |
| ARM64 audio codec | minimp3 decodes all 26 music tracks + voice (qemu) |
| Real device | **runs on the user's Samsung Galaxy S25** (0.1.3: city simulation, date and speed advance, 60 FPS, user report 2026-10-22); 0.1.4–0.1.6 confirmed by the user (zoom, lists, speed, preview placement, settings, music/sound, save → kill → load, home/screen-off resume); 0.1.7 not yet tried |

## Playable features (desktop-verified unless noted)

Main menu, leader roster, adventure selection, mission briefing, city view, simulation,
events, camera pan/zoom via touch gestures. Android-specific features (importer, soft
keyboard, touch bar, lifecycle, 60 FPS render split, UI scale): importer, data download,
game start, large UI, touch bar, 60 FPS confirmed by the user on a phone; the rest is
desktop-verified only.

## Highest-priority problems

1. On-device: army controls (0.1.7) with real soldiers; long session / memory measurement.
2. Desktop re-test done for all engine changes up to 0.1.4 (TEST_REPORT).
3. Toolchain provenance (third-party NDK sysroot / android.jar / d8) – rebuild with official
   tools before any distribution.
4. Possible high GPU memory use on phones (interface + i30 + i15 packs ≈ 1.15 GB RGBA if all
   sprites get loaded).

## Parity coverage (honest)

0 systems `MATCHED` against the original game. Several systems confirmed to *run* in the
port (PARITY_MATRIX.md). The only gameplay-visible change made: text fields accept `q`.
