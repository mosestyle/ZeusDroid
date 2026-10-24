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
| Tap | map, build tool active (default **preview + confirm**, since 0.1.5) | shows a green/red preview of the building at the tap; nothing is built. Tap elsewhere to move it |
| Drag starting on the preview | map, preview shown | moves the preview tile by tile (it keeps its offset to the finger) |
| One-finger drag elsewhere | map, build tool active | pans; the preview stays on its tile |
| Hold still 300 ms, then drag | map, build tool active | previews a road / wall / row / area from the hold point to the finger; on lift it stays a preview. Drag its end to resize |
| ✓ / ✕ buttons next to the preview | map | ✓ builds exactly what the preview shows (price shown under ✓ for single buildings); ✕ discards the preview. On a red preview ✓ builds nothing and the preview stays |
| (after ✓, single building) | map | "Next one ready" (0.1.6, `build_repeat`): a new preview appears beside the building — one building width along the map's x axis, or with the same spacing/direction as the last two confirmed buildings. A manual tap starts a new row |
| **Draw** button (touch bar, while a tool is selected; on by default for roads, walls, housing, parks, trees, animals, columns, demolish) | map | one-finger drag draws the row / area preview at once from the touch-down point (no hold); two fingers pan / zoom. Tap still places a single preview. Remembered per tool for the session |
| Draw a road (Draw on) | map | freehand: the road follows the finger's path with turns (tiles in order, gaps filled 4-connected); moving back over it erases; red = cannot build; ✓ shows the total price. Dragging the end of the preview continues the road |
| Finger near the map edge while drawing / dragging a preview | map | the map scrolls (up to ~900 px/s, faster closer to the edge) and the preview follows the finger |
| Tap outside an info window | map | closes it |
| (instant mode, `build_confirm "off"`) tap / hold + drag | map, build tool active | 0.1.3 behaviour: tap builds at once; hold then drag draws and builds on lift |
| Long press (500 ms, no movement) | map, no build tool | right click (close info, deselect / place army) |
| **X** button (appears when needed) | touch bar | discards a preview, else leaves the build tool, closes an info panel, ends patrol editing, clears army selection |
| Android Back | in the city | same as **X** when something is active, otherwise opens the game menu (with **Touch controls**) |
| Two-finger pinch | map | continuous zoom 0.5×–3× (since 0.1.4); the map points under the fingers stay under them. Ends near 1× snap to the pixel-exact 1× view |
| Two-finger drag | map (also in build mode) | pans the camera; finger-distance changes under 4 % (`pinch_deadband`) do not zoom |
| Tap | list (adventures, leaders, saves) | selects the row under the finger; rows are ≥ 1/13 of the screen height on touch screens |
| One-finger drag | list | scrolls the list (no selection) |
| Android Back | anywhere | Escape (closes dialogs/menus, cancels) |
| Tap on a banner (flag) or next to it | map | selects that army unit (replaces the selection) |
| Tap on the map, army selected | map | move order to that tile (right click on desktop); the selection stays |
| **[ ]** button, then one-finger drag | touch bar (only when the city has soldiers) | selection rectangle; all banners inside are selected; the mode ends after the drag |
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
build_confirm "on"       # on: preview + confirm button; off: taps build at once
```
Since 0.1.5 these can be changed in the game: **Back → Touch controls** in a city, or
**Options → Touch controls** in the main menu (each value is a button that cycles through the
choices; changes are saved and applied at once, interface size after a restart). On phones the
Options screen hides the resolution and full-screen buttons, which have no effect there.

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

## Drawing rows, areas and roads (0.1.16)
* Default: a one-finger drag pans the map, even with a tool selected. Tap → preview; **Draw**
  beside ✓ / ✕ → the next one-finger drag anywhere draws from the preview (smart straight
  roads); ✓ builds, ✕ cancels. Hold-then-drag also still draws.
* Option "Drawing rows, areas, roads" → "Draw switch on the bar": the older behaviour (a
  "Draw" toggle on the touch bar; with it on, a one-finger drag draws at once).
* Port setting `draw_style` = `button` (default) | `switch`; test override
  `EZEUS_DRAW_STYLE`.

## Save / Load / full screen (0.1.12–0.1.14)
* Right edge of the map: **Save** (first time asks for a name, then quick-saves to the same
  file), **Load**, **>>** full-screen city view (side panel slides away; **<<** brings it
  back).
* The keyboard opens only when the name field is tapped; Enter closes it.
* Rotate icon next to ✓ / ✕ for buildings that can be rotated (0.1.15).
* Undo icon on the left bar for 30 s after building: removes the last placement and refunds
  it; the right panel's Undo does the same (0.1.15).
* Autosave as "autosave" whenever the app goes to the background.
