# Performance and timing

## Architecture (implemented)

| Concern | Upstream eZeus | This port |
|---|---|---|
| Paint rate | fixed 20 FPS (`emainwindow.cpp`, `sleep_for`) | 20 FPS desktop default; 60 FPS on Android (`EZEUS_RENDER_FPS` overrides, 20–120) |
| Simulation | `incTime`/`incFrame` once per painted frame | once per 20 Hz tick (`platform/eframeclock`); identical to upstream when render FPS = 20 |
| Sprite animation frames | per painted frame | per tick |
| Edge scrolling | per frame, ±35 px | per tick (same speed); disabled on touch |
| Music scheduler (`eMusic::incTime`) | per frame | per tick |
| Input sampling | per frame (20 Hz) | per frame (60 Hz) – touch pan/zoom react 3× faster |
| Catch-up | none (slow frames slow the game) | none (same) |

Expected effect: camera and UI respond at 60 Hz while the city, walkers and animations
advance exactly 20 times per second. Walkers are not interpolated between ticks (they move
in the same steps as on the desktop).

## Measurements

None on Android yet (no device attached to the development environment). Required:
startup time, steady FPS while panning, memory with i15+i30 (≈230 MB of PNG packs decoded
to textures), late-game city. See NEXT_STEPS.md.

Desktop (Xvfb, software rendering, 2 vCPU container) is **not representative** of phone GPU
performance; only functional timing checks are done there.
