# Desktop builds (Windows and Linux)

The desktop build is the parity/debugging reference and must keep working.

## Linux (verified)

```
sudo apt install cmake ninja-build g++ libsdl2-dev libsdl2-image-dev \
    libsdl2-mixer-dev libsdl2-ttf-dev libnoise-dev xvfb
cmake -S engine -B build/desktop -G Ninja -DCMAKE_BUILD_TYPE=Release -DEZEUS_SYSTEM_SDL=ON
ninja -C build/desktop
python3 tools/assets/stage_dev_assets.py --game "/path/Zeus and Poseidon" \
    --ezeus-zip eZeus-0.8.2-beta.zip --out local-assets
EZEUS_BIN_DIR=$PWD/local-assets/ZeusPoseidon/eZeus/Bin/ build/desktop/eZeus
```
Verified on Ubuntu 24.04 (GCC 13.3, SDL2 2.30 system packages): builds and runs with the
user's GOG data (TEST_REPORT.md).

Useful environment variables:

| Variable | Effect |
|---|---|
| `EZEUS_BIN_DIR` | directory playing the role of `eZeus/Bin/` |
| `EZEUS_TOUCH=1` | enable the touch gesture layer and touch button bar on desktop |
| `EZEUS_RENDER_FPS` | 20 (default desktop, identical to upstream) … 120 |
| `EZEUS_AUTOTEST` / `EZEUS_AUTOTEST_LOG` | scripted input + screenshots (see `engine/platform/eautotest.h`) |
| `SDL_AUDIODRIVER=dummy` | headless runs |

Headless automation: `tools/tests/run_desktop_autotest.sh <script> <log> [VAR=value…]`.

## Windows (not yet verified here)

No Windows toolchain is available in the development environment (the linked PC exposes
files only, no shell). Expected procedure with Visual Studio 2022 + CMake:

```
cmake -S engine -B build\win -G "Visual Studio 17 2022" -A x64
cmake --build build\win --config Release
```
FetchContent builds SDL2 and its satellites from source; the new `SDL2/` compat include
directory (engine/CMakeLists.txt) removes the dependency on a pre-arranged SDL include tree.
Copy the resulting `eZeus.exe` and SDL DLLs into `<Zeus and Poseidon>\eZeus-0.8.2-beta\Bin\`
(the layout described in GAME_DATA.md) – never into the original game folder itself.
