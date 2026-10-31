# ZeusDroid – Zeus + Poseidon for Android (native port, built on the eZeus engine)

A native **arm64-v8a** Android build of [eZeus](https://github.com/MaurycyLiebner/eZeus),
the open-source C++/SDL2 reimplementation of *Zeus: Master of Olympus* + *Poseidon*,
with a touch-first input layer. No emulation (no Box64/Wine/Winlator), no game content
in the APK: you import your own original game files.

> Status: development build, played on a Samsung Galaxy S25. See `PROJECT_STATE.md` for
> what is verified and `docs/RELEASE_NOTES.md` for the changes per version.

**Download:** the APK is attached to each version on the
[Releases page](https://github.com/mosestyle/ZeusDroid/releases).

## What is in this repository

| Path | Content |
|---|---|
| `engine/` | eZeus (GPL-3.0) pinned at `ec765199`, plus port changes (`git diff 0416715 -- engine/`) |
| `engine/platform/` | Android/desktop platform layer: paths, ENG→XML converter, touch gestures, frame clock, autotest driver |
| `android/` | Manifest, SDL2 Java glue, first-run importer (`ImportActivity`), `ZeusActivity`, Gradle files |
| `tools/build_android.sh` | Gradle-free APK build (used in CI-like environments) |
| `tools/assets/` | `eng_to_xml.py`, `stage_dev_assets.py`, `validate_game_data.py`, `asset_manifest.json` |
| `tools/tests/` | unit-test runner, headless desktop autotest runner |
| `tests/` | native (C++), Java and Python tests |
| `docs/` | audit, build, data, touch, parity, performance, test report |

## Quick start (player)

1. Install the APK from the Releases page (or `adb install -r ZeusDroid-debug-arm64-v8a.apk`).
2. Copy your own **Zeus and Poseidon** install folder (e.g. the GOG edition) to the phone.
3. Start *ZeusDroid* → *Import game folder…* → *Start game*. The first start prepares the
   game graphics from your own game files (about a minute, once; see
   `docs/TEXTURE_BUILDER.md`). Nothing else is downloaded.

Details: `docs/GAME_DATA.md`. Controls: `docs/TOUCH_CONTROLS.md`.

## Quick start (developer)

```
# desktop (Linux) reference build + headless tests
cmake -S engine -B build/desktop -G Ninja -DCMAKE_BUILD_TYPE=Release -DEZEUS_SYSTEM_SDL=ON
ninja -C build/desktop
tools/tests/run_unit_tests.sh

# Android APK
tools/build_android.sh            # or: cd android && ./gradlew assembleDebug
```
See `docs/BUILD_ANDROID.md` and `docs/BUILD_WINDOWS.md`.

## Licence

GPL-3.0 (inherits eZeus). Third-party notices: `LICENSES/NOTICE.md`. Zeus: Master of
Olympus and Poseidon are trademarks/copyrighted works of their owners; this project is not
affiliated with them or with the eZeus authors.
