# Touch controls

Implementation: `engine/platform/etouchgestures.{h,cpp}` (SDL-independent state machine),
integrated in `engine/emainwindow.cpp` (`eWindowTouchSink`, `handleFingerEvent`).
Enabled automatically on Android; on desktop set `EZEUS_TOUCH=1` (used by the autotests).
SDL's synthetic touch→mouse and mouse→touch events are disabled on Android
(`SDL_HINT_TOUCH_MOUSE_EVENTS=0`, `SDL_HINT_MOUSE_TOUCH_EVENTS=0`) and any remaining
`SDL_TOUCH_MOUSEID` mouse events are dropped, so a tap is never dispatched twice.

## Gestures

| Gesture | Where | Result in the game |
|---|---|---|
| Tap | map | hover + left click at the touch-down point (select, open building/walker info, place building) |
| Tap / drag | any UI panel, dialog, menu, slider, list | immediate left press, drag = left-drag, lift = release (exactly like a mouse) |
| One-finger drag | map, no build tool | pans the camera (middle-drag in eZeus); the map stays under the finger, sub-pixel motion is accumulated |
| One-finger drag | map, build tool active | **pans** (default "hold-to-draw" mode, since 0.1.3) |
| Tap | map, build tool active | places one building / road tile at the tap |
| Hold still 300 ms, then drag | map, build tool active | draws roads/walls, places rows/areas of buildings from the hold point; lift = build |
| Long press (500 ms, no movement) | map, no build tool | right click (close info, deselect / place army) |
| **X** button (appears when needed) | touch bar | leaves the build tool, closes an info panel, ends patrol editing, clears army selection |
| Android Back | in the city | same as **X** when something is active, otherwise opens the game menu |
| Two-finger pinch | map | continuous zoom 0.5×–3× (since 0.1.4); the map points under the fingers stay under them. Ends near 1× snap to the pixel-exact 1× view |
| Two-finger drag | map (also in build mode) | pans the camera; finger-distance changes under 4 % (`pinch_deadband`) do not zoom |
| Tap | list (adventures, leaders, saves) | selects the row under the finger; rows are ≥ 1/13 of the screen height on touch screens |
| One-finger drag | list | scrolls the list (no selection) |
| Android Back | anywhere | Escape (closes dialogs/menus, cancels) |
| Soft keyboard | when a text field is focused (leader name, save name) | shown automatically, hidden when the field loses focus |

Safety rules built into the state machine:

* Nothing is sent to the map until the gesture is classified, so panning can never build,
  demolish or select by accident.
* A second finger that lands before the first one moved turns the gesture into pinch/pan;
  no click is sent. In build mode this means two-finger panning never places anything.
* When one finger of a pinch lifts, the remaining finger is ignored until all fingers are
  up (no camera jump, no stray tap).
* Gestures that start on a UI element stay UI interactions (a drag off a panel never pans).
* App backgrounding/focus loss cancels the gesture: held buttons are released, build drags
  are cancelled with a right click first so no road/wall is committed.
* Mouse edge-scrolling is disabled on touch (the last touch point is not a resting cursor).

Tunables live in `<eZeus dir>/port_settings.txt` (created with defaults on first start; on
Android: `Android/data/org.zeusport.game/files/ZeusPoseidon/eZeus/port_settings.txt`):

```
render_fps "auto"        # 20 = upstream timing of paint; 60 default on Android
ui_scale "auto"          # auto | tiny | small | medium | large (auto: 1080p+ phones -> large)
pan_sensitivity "1.0"
long_press_ms "500"
pinch_deadband "1.04"     # finger-distance ratio before a two-finger drag zooms
max_zoom "3.0"           # 1..4; maximum pinch / mouse-wheel zoom
tap_slop_px "24"
build_drag "hold"        # hold (default) | direct (0.1.0-0.1.2 behaviour: drag draws immediately)
build_hold_ms "300"
```
An in-game settings screen for these is still open (NEXT_STEPS.md).

## On-screen buttons

eZeus has some actions only on the keyboard. On touch screens four framed buttons are
added at the top-left of the map: `||` pause/resume (P), `-` / `+` game speed ([ / ]),
between them the current speed (`0.2x`, `1x`, `2.5x`, `5x`, `10x`, `50x`; tap it to return to
`1x`), `R` rotate the view (R), and `X` (cancel, shown only when a build tool, info panel,
patrol edit or army selection is active). (0.1.3 showed a "Speed Nx" tip per tap instead; the
tips stacked up and hid the current value.)

## Zoom: texture sets plus world scaling (since 0.1.4)

eZeus draws from pre-scaled texture sets (tile width 30/60/90/120 px = `i15/i30/i45/i60.e`);
the default pack has `i15` and `i30`, i.e. only two zoom levels, both small on a phone.
The port adds a world-layer scale (`eGameWidget::paintEvent` → `paintWorld`): when the zoom is
not 1×, the map is drawn 1:1 into an off-screen texture of (screen size ÷ scale) and drawn
stretched with linear filtering; menus, panels and the touch bar are not scaled. Input on the
map is divided by the same scale (`toWorld`), so taps, hold-to-draw and info clicks land on the
tile under the finger at every zoom. The texture set is chosen as the largest one that needs at
most a 0.75× downscale (zoom 0.5–0.75 uses `i15`, above uses `i30`, or `i45`/`i60` if installed).
At exactly 1× nothing changes compared with upstream (no off-screen pass). Desktop mouse wheel
steps through 0.5, 0.75, 1, 1.5, 2, 2.5, 3 anchored at the cursor (upstream: texture sets only,
anchored at the screen centre). Zoom is not stored in saves (save format unchanged).

## Tests

* Unit tests: `tests/native/test_touch_gestures.cpp` – 14 scenarios, 74 checks (tap jitter,
  long pan, sub-pixel pan, long-press, continuous pinch in/out and pinning, second finger
  joining and leaving, UI vs map, build placement and road drag, hold-to-draw, two-finger
  pan in build mode, two-finger pan dead band, list tap vs drag-scroll, lifecycle cancel,
  third finger). Run on x86-64 and as a static ARM64 Android binary under
  qemu (`tools/tests/run_unit_tests.sh`).
* Engine integration: `build/run/touch1.txt` autotest drives SDL finger events through the
  real game (desktop, Xvfb). Logged camera state confirmed pan (-300, -150) px exactly, a
  zoom-out step anchored at the pinch centre, a jittery tap that did not move the camera,
  and that a pinch starting on a message box is treated as a UI touch.
* Not yet done: tests on a physical phone (no device is attached to this environment).
