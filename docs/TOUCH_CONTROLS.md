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
| One-finger drag | map, build tool active (roads, walls, building placement, erase) | left-drag starting at the touch-down point (road/wall/area drawing) |
| Long press (500 ms, no movement) | map | right click (eZeus/Zeus: close info, cancel build tool, deselect / place army) |
| Two-finger pinch | map | one zoom step per 30 % change in finger distance, anchored at the pinch centre |
| Two-finger drag | map (also in build mode) | pans the camera |
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

Tunables (`eTouchConfig`): tap slop 24 px, long-press 500 ms, pinch step ratio 1.30, pan
sensitivity 1.0. An in-game settings screen for these is still open (NEXT_STEPS.md).

## Zoom: discrete, by design of the renderer

eZeus draws from pre-scaled texture sets (tile width 30/60/90/120 px = `i15/i30/i45/i60.e`).
Pinch therefore moves between the installed sets instead of scaling continuously; with only
the default pack (`i15.e`, `i30.e`) there are two zoom levels. Installing `i45.zip`/`i60.zip`
adds two closer levels. Continuous zoom would need either GPU scaling between levels (blurry
pixel art, against the fidelity goal) or a renderer change; recorded as an open item.

## Tests

* Unit tests: `tests/native/test_touch_gestures.cpp` – 11 scenarios, 50 checks (tap jitter,
  long pan, sub-pixel pan, long-press, pinch in/out anchoring, second finger joining and
  leaving, UI vs map, build placement and road drag, two-finger pan in build mode,
  lifecycle cancel, third finger). Run on x86-64 and as a static ARM64 Android binary under
  qemu (`tools/tests/run_unit_tests.sh`).
* Engine integration: `build/run/touch1.txt` autotest drives SDL finger events through the
  real game (desktop, Xvfb). Logged camera state confirmed pan (-300, -150) px exactly, a
  zoom-out step anchored at the pinch centre, a jittery tap that did not move the camera,
  and that a pinch starting on a message box is treated as a UI touch.
* Not yet done: tests on a physical phone (no device is attached to this environment).
