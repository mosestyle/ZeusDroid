# Release notes

## 0.1.3-dev
* Build tools: one-finger drag pans; tap places; hold 300 ms then drag draws roads/rows.
  Old behaviour available with `build_drag "direct"` in port_settings.txt.
* New `X` touch button (only visible when useful) and Back = cancel build tool / info panel.
* Speed buttons show a "Speed Nx" tip.
* App version shown on the import screen.

## 0.1.2-dev
* Fix: touch-bar buttons froze the city simulation.

## 0.1.1-dev
* Importer rejects GitHub's source-code zip; in-app verified download of the eZeus data pack.

## 0.1.0-dev (2026-10-22) – first native Android development build

Engine: eZeus `ec765199` (2026-10-19) + port changes. Debug-signed, arm64-v8a only.

New for Android
* Native `libmain.so` (eZeus) + SDL2 2.32.10 family, launched from SDLActivity.
* First-run importer (Storage Access Framework): game folder and eZeus data pack, validation,
  resumable atomic copies, saves preserved, licences screen, "Manage game data" shortcut.
* Built-in `.eng → XML` conversion (no Windows converter needed).
* Touch: tap / long-press / one-finger pan / pinch zoom anchored at the fingers / two-finger
  pan / build drags; Back = Escape; soft keyboard for text fields; pause/speed/rotate buttons.
* 60 FPS rendering with the simulation kept at 20 ticks per second.
* Large UI scale on 1080p+ phones; `port_settings.txt` for tuning.
* Crash-safe saves (temp file + rename, `.bak` of the previous save).
* stdout/stderr of the engine forwarded to logcat (tag `eZeus`).

Fixes to upstream behaviour
* Text fields accepted every letter except `q`.

Known limitations
* Not yet run on a physical device or emulator (none available to the build environment).
* Zoom is stepwise (eZeus texture sets), not continuous.
* No touch equivalent yet for soldier box-selection.
* GPU texture memory on low-RAM phones unmeasured.
* GL context loss after resume would leave textures invalid (eZeus does not reload them).
* Built with a partly third-party toolchain (see docs/BUILD_ANDROID.md); rebuild with the
  official NDK/SDK before distributing.
