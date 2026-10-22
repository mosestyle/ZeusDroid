# eZeus engine audit

Upstream: https://github.com/MaurycyLiebner/eZeus, pinned at
`ec765199f9719d1724fe07ebd776a2240c099dcd` (main, 2026-10-19, "Merge pull request #80").
Imported unmodified as commit `0416715` in this repo (`engine/`); every later change
is visible with `git diff 0416715 -- engine/`.

Comparison fork: https://github.com/gitTerebi/Olympus at `161a1226a430628cb6aefae0bd84b4fbae5c5f82`
(2026-07-04), checked out read-only in `third_party/Olympus` (not committed).

All file references below are relative to `engine/` at the pinned revision unless noted.

## Summary

| Area | Finding (evidence) | Impact on Android |
|---|---|---|
| Build | `CMakeLists.txt`: single desktop `add_executable(eZeus …)` with ~700 sources; SDL2 2.32.10, SDL2_ttf 2.24.0, SDL2_image 2.8.12, SDL2_mixer 2.8.2 fetched with FetchContent. `eZeus.pro` (qmake, `CONFIG -= qt`) hard-codes the author's Windows library paths. libnoise linked only `if(LINUX)`. | **Changed:** shared source list, `add_library(eZeus SHARED)` + `OUTPUT_NAME main` on Android, `EZEUS_SYSTEM_SDL` option for desktop, `GIT_SUBMODULES` limited (SDL_image's recursive libjxl submodules fail to clone), `SDL2/` compat include dir (engine includes `<SDL2/SDL_mixer.h>`, source builds install `SDL_mixer.h`). |
| Licences | eZeus GPL-3.0 (`LICENSE.md`); SDL2 + satellites zlib; freetype FTL/GPL2 (vendored by SDL_ttf); stb/minimp3/dr_flac public domain or MIT-0 in SDL_image/SDL_mixer. | APK must ship GPL source offer + notices (see `LICENSES/`). |
| Entry point | `main.cpp` defines `SDL_MAIN_HANDLED` and `int main()`. | **Changed:** `SDL_MAIN_HANDLED` only on desktop; `main(int, char**)` so SDL_main.h maps it to `SDL_main` for SDLActivity. |
| Paths | `egamedir.cpp`: everything is relative to `SDL_GetBasePath()` (game root = `Bin/../../`, eZeus files = `Bin/../`). `SDL_GetBasePath()` returns NULL on Android → `std::string(nullptr)` crash. | **Changed:** `platform/eplatformpaths.cpp` resolves the "Bin" dir: `EZEUS_BIN_DIR` env → Android `<external files>/ZeusPoseidon/eZeus/Bin/` → SDL base path. Desktop layout is kept on Android so no game code needed changes. |
| Text data | `elanguage.cpp` reads `Zeus_Text.xml`/`Zeus_MM.xml` produced by the Windows-only `engconverter-0.5.exe` (UPX-packed Qt4 build of MIT-licensed bvschaik/citybuilding-tools). Parser `exmlparser.cpp` is line-based (one element per line). | **Added:** portable converters (`tools/assets/eng_to_xml.py`, native `platform/eengconverter.cpp`, run automatically at first start). Both produce byte-identical output on the user's files. |
| Rendering | SDL_Renderer, `SDL_RENDERER_ACCELERATED`, no vsync; 14 render-target uses; `SDL_HINT_RENDER_SCALE_QUALITY=1`. Textures are pre-scaled per zoom level (`i15/i30/i45/i60.e`, tile widths 30/60/90/120 px). `widgets/egamewidget.cpp::setTileSize` rescales `mDX/mDY` around the widget centre. | Android uses the GLES2 SDL renderer (hint set). Render targets are re-created on `SDL_RENDER_TARGETS_RESET` (already handled in `emainwindow.cpp`). Zoom is **discrete** (texture sets) – see TOUCH_CONTROLS.md. |
| Timing | `eMainWindow::exec`: fixed `fpsClamp = 20`, `sleep_for(50ms - frame)`. The simulation advances inside `eGameWidget::paintEvent` (`widgets/egamewidgetpaint.cpp`: `mBoard->incFrame()`, `mBoard->incTime(mSpeed)` once per frame, 5 iterations at max speed). Animation frames (`mFrame`) and mouse edge-scroll (±35 px) are also per frame. | **Changed:** `platform/eframeclock` marks which painted frames are simulation ticks. With render FPS 20 (desktop default) every frame ticks = upstream behaviour. Android renders at 60 FPS; simulation/animation/edge-scroll/music timing only advance on 20 Hz ticks; no catch-up bursts (slow devices slow the game like upstream). |
| Input | `eMainWindow::exec` maps SDL mouse events to `eMouseEvent` (left/right/middle) and dispatches to the widget tree (`widgets/ewidget.cpp::mouseEvent`, grabbers, `sLastPressed`). Middle-drag pans the map (`eGameWidget::mouseMoveEvent`), wheel changes tile size, keys via scancodes. Text entry `widgets/elineedit.cpp` uses scancodes only (no SDL_TEXTINPUT; upstream bug: no `q`). | **Added:** `platform/etouchgestures` FSM → synthetic left/right/middle events; SDL synthetic touch↔mouse disabled; Back → Escape; soft keyboard shown while an `eLineEdit` grabs the keyboard; edge-scroll disabled on touch; `q` fixed. |
| Map & camera | `pixToId` (`egamewidget.cpp:540`) is the isometric inverse; camera = `mDX/mDY` pixel offset clamped by `clampViewBox()`. | `zoomStepAt()` re-anchors the zoom at the pinch centroid. |
| UI | Custom widget tree, resolution-dependent UI scale (`widgets/eresolution.cpp`: height ≤800 → 1.0, ≤1200 → 1.5, else 2.0). | Android uses native display size, so a 2400×1080 phone gets 1.5× UI. Separate UI-scale setting is a NEXT_STEPS item. |
| Game logic | ~600 files: walkers (`characters/`), buildings, gods/heroes/monsters, campaigns from original `.pak` (`engine/ecampaignreadpak.cpp`), events (`gameEvents/`), AI cities. README states it is *not* an exact copy. | Unchanged. Parity tracked in PARITY_MATRIX.md. |
| Assets | Original sprites read directly from `DATA/*.sg3 + *.555` (`textures/egametextures.cpp` lists expected sizes); interface/scaled sprites from eZeus `.e` packs (`esplitbinary.h`: name → (file, offset, size) PNG table; unchanged since tag 0.8.2-beta, so the 0.8.2-beta packs match this revision). Audio from `Audio/` (wav/mp3), adventure voices from `Adventures/<name>/`. | Importer copies user-owned files; `.e` packs come from the user's own download of the eZeus release. |
| Audio | SDL_mixer `Mix_OpenAudio(44100, default, 2, 2048)`, `Mix_LoadMUS` mp3 music, `Mix_LoadWAV`. Missing files only log. Music retry spam when files are missing (`emusic.cpp`). | Android build uses SDL_mixer's minimp3 + built-in WAV; opus/mod/midi-fluidsynth/wavpack disabled. Pause/resume via `SDL_HINT_ANDROID_BLOCK_ON_PAUSE(_PAUSEAUDIO)`. |
| Save/load | `fileIO/ewritestream.h`: all integral types (incl. `size_t`) are written through a template that casts to `int32_t`; `bool`=1 byte, `float`/`double` raw IEEE; no pointers written. Saves live in `Bin/../Save/<leader>/`. | Little-endian on both x86-64 and arm64 → desktop and Android saves should be interchangeable (**not yet tested on device**). Original Zeus `.sav` files are a different format and are not loaded by eZeus. |
| Portability | Only platform `#ifdef`s: `__unix__` (libnoise map generator; Android also defines `__unix__`) and `_WIN32` in `elimits.h`. `std::filesystem`, `std::thread` (`engine/ethreadpool.cpp`, 2–4 workers). | Map generator (editor-only feature, not in original game) disabled on Android. |
| Memory | `i30.e` 150 MB, `i15.e` 80 MB, `interface.e` 85 MB of PNGs decoded to textures at start-up (all sizes enabled by default). | Needs on-device profiling; RAM on 4 GB phones is a risk (NEXT_STEPS). |
| Startup | Window from `settings.txt` (default 1280×720 windowed). | Android: fullscreen at native resolution (`ePlatform::adjustSettings`). |

## Olympus fork (comparison only)

Olympus (GPL-3.0, AI-assisted per its README, ~50 commits on a restructured `src/` tree)
adds hotkeys, smooth scrolling, a 240 FPS mode, non-blocking toasts and several gameplay
changes (settlers in groups of 4, gradual house devolution, homeless walkers, "god
blessings match original zeus"). None has been merged: each gameplay change must first be
reproduced in the original game and logged in PARITY_MATRIX.md, and the tree layout differs
too much for blind cherry-picks.

## Android build strategy (as implemented)

1. Same CMake project builds `libmain.so` + `libSDL2*.so` for `arm64-v8a`, API 26, static libc++.
2. SDL2 2.32.10's own `SDLActivity` Java glue (copied to `android/app/src/main/java/org/libsdl/app`).
3. `ImportActivity` (launcher) validates/imports user data, then starts `ZeusActivity extends SDLActivity`.
4. No Box86/Box64/Wine/emulation: everything is native AArch64 code (verified per-library by the packaging script).
