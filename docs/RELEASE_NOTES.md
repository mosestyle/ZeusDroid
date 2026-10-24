# Release notes

## 0.1.10-dev
* Desirability now works like the original game: every building spreads its value in
  square rings with the original steps (from your Model files), and houses take the best
  value under them. Houses evolve and devolve with the original thresholds for your
  difficulty (with the original "buffer" between devolving and evolving). This changes how
  attractive areas must be, so existing cities may settle at different house levels.
* Houses must touch a road, as in the original: plots that don't are removed and the tip
  "Some plots are too far from a road" is shown.
* Settlers arrive in groups of up to 4 (as observed in the original).
* Roads look paved above the original desirability limit.

## 0.1.9-dev
* Freehand roads: with Draw on, the road follows your finger, including turns; going back
  over it erases it; red tiles cannot be built; the price of the whole road is shown on ✓.
* Edge auto-scroll: drawing (or dragging a preview) near the edge of the map scrolls the map,
  so long roads and big areas no longer need several drags.
* Short vibration when something is built (Touch controls → Vibrate when building).
* Gameplay parity with the original (Phase F): building costs per difficulty, worker counts
  and desirability values are now read from your original game's Model files; fixed "Very
  Low"/"Low" tax rates being swapped. Details: docs/PARITY_MATRIX.md.

## 0.1.8-dev
* **Draw** button on the touch bar while a building tool is selected: with Draw on, a
  one-finger drag draws the road / wall / row / area straight away (no hold), two fingers
  move the map. On by default for roads, walls, housing, parks, trees, animals, columns and
  demolish; off for single buildings. Remembered per tool during the game.
* Optional overlay with FPS, frame time, simulation ticks per second and memory
  (Touch controls → Show FPS and memory).
* Tap on the map outside an info window closes it.
* Lists keep a phone-sized row height on tablets; the Touch controls screen fits the screen.

## 0.1.7-dev
* Army on touch screens: tap a banner (flag) to select it, tap the map to send it there
  (the selection stays for more orders), "[ ]" on the touch bar + drag to select several
  banners with a rectangle, X / Back to deselect. "[ ]" only shows when the city has soldiers.

## 0.1.6-dev
* "Next one ready": after ✓ on a single building the next preview appears right beside it;
  move it once and the following ones keep that spacing and direction (rows of farms,
  houses, fountains with one tap each). Setting: Touch controls → After building.
* The ✓ button shows the price of a single building.
* ✓ on a red preview (not allowed there) keeps the preview so it can be moved.

## 0.1.5-dev
* Two-step building placement (default): a tap shows a preview, drag the preview to nudge
  it, ✓ builds and ✕ discards. Hold + drag previews roads, walls and rows/areas of buildings
  before they are built; the tool stays selected for the next placement.
* In-game **Touch controls** screen (Back → Touch controls, or Options → Touch controls):
  placement mode, hold time, long-press time, drag speed, maximum zoom, frame rate, UI size.
* Phone Options screen no longer shows resolution / full-screen buttons (no effect on phones).
* Back is routed to the centre of the screen, so it opens/closes menus regardless of the
  last tapped button.

## 0.1.4-dev
* Continuous pinch zoom from 0.5× to 3× (was: two fixed levels); the map stays under the
  fingers, and a two-finger drag pans without zooming. Mouse wheel uses the same zoom range.
* Current game speed shown between `-` and `+` (tap it for 1×); the stacking "Speed" tips
  are gone.
* Lists (adventures, leaders, save/load) have finger-sized rows and scroll by dragging.
* Port settings: `pinch_deadband`, `max_zoom` (`pinch_step` is obsolete).

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
