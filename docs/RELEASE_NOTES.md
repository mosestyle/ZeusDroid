# Release notes

## 0.1.19-dev
* **Texture builder**: the game graphics are now made from your own original game files
  (`DATA/*.sg3`, `*.555`, `*.jpg`) on the first start – "Preparing the game graphics from
  your game files" with a progress bar, about a minute, only once. The texture pack
  download from GitHub is no longer needed (it stays as an optional choice in the importer).
  The normal zoom size is the original pixels exactly; the small zoom size is scaled down.
  See docs/TEXTURE_BUILDER.md.
* Importer: only the game folder is needed; the checklist says "Game graphics (prepared
  from your game files on first start)".
* The fonts, language file and ZeusDroid's own adventures now come with the app.

## 0.1.18-dev
* Undo is now a plain **Undo** button in the right-hand column (under ">>"), away from the
  camera notch, in the same style as Save / Load / Menu. It disappears after 10 seconds by
  default; Touch control options → "Undo button stays for": 10 seconds / 30 seconds /
  1 minute.
* Mini map (Overview panel): dragging moves the view smoothly with the finger instead of
  jumping around.
* Import screen: "Save a backup of the texture pack (.zip)…" writes the imported texture pack
  to a file you choose (phone, memory card, cloud). "Import texture pack" sets the game up
  again from that file, so the download from GitHub is never needed again.

## 0.1.17-dev
* The app is now called **ZeusDroid** (icon label, title, importer). The importer speaks of
  the "texture pack"; only the pack's file name (eZeus-0.8.2-beta.zip) is still shown so you
  can find it. Internal folders keep their names, so imports and saves stay where they are.
  APK file: ZeusDroid-debug-arm64-v8a.apk.
* **Menu** button under Save / Load opens the game menu (Resume, Save, Load, Touch control
  options, Exit) without the Back gesture.
* **Custom Adventures** button beside New Adventure (the original's own text). Like in the
  original, adventures in their own folder (Open Play Economic/Military/Sandbox 2, The
  Odyssey, The Founding of Troy, The Mayan Revolution, The Sinking of Atlantis, The Youngest
  Twins) and eZeus' two own adventures are listed there, alphabetically; New Adventure shows
  the original campaigns and Open Play maps.
* Options on the phone: a compact window with the texture sizes and Touch control options
  (the button no longer sticks out of the frame). Sizes whose texture pack is not imported
  say "(not installed)" instead of showing no tick box.

## 0.1.16-dev
* New default way to draw rows, areas and roads: a one-finger drag always moves the map, also
  with a building tool selected. Tap to place a preview, then tap **Draw** next to ✓ / ✕:
  the next drag (anywhere on the map) draws the row / area / road from the preview; drag
  again to change it, ✓ builds. Holding still before dragging still draws too.
* The old "Draw" switch on the left bar is still available: Touch control options →
  "Drawing rows, areas, roads" → "Draw switch on the bar".
* "Touch controls" renamed to "Touch control options".

## 0.1.15-dev
* Rotate moved next to ✓ / ✕: a rotate icon (the original's rotate arrow) appears beside ✕
  while placing a building that can be rotated (stadium, palace, gatehouse, horse ranch,
  sanctuaries, pyramids, monuments). The "R" on the left bar is gone.
* Undo: after building, an undo icon (the original's Undo arrow) appears on the left bar for
  30 seconds; it removes everything placed by the last ✓ (one building, a row, a road) and
  gives the money back. The Undo button in the right panel (round arrow, next to demolish)
  now works too. Demolishing cannot be undone.

## 0.1.14-dev
* Full-screen city view: the ">>" button under Save / Load slides the right panel away so the
  city fills the screen; "<<" brings it back. The selected building tool keeps working.
* Loading a save puts the camera back where it was, including the zoom (stored in a small
  "<save>.view" file next to the save; older saves load at normal zoom).
* The "R" button (rotate) now only appears while placing a building that can be rotated:
  stadium, palace, gatehouse, horse ranch, sanctuaries, pyramids and monuments. Before it
  was always shown but only did something for those buildings.

## 0.1.13-dev
* Save / Load dialogs no longer open the phone keyboard by themselves: it opens when you tap
  the name field, and Enter closes it. (Other text fields, e.g. a new leader's name, still
  open it at once because you must type there.)

## 0.1.12-dev
* Smart roads: a drawn road is made of straight lines. Wobbles and slow drift of the finger
  are ignored; a corner is made only when you clearly move sideways. Going back along the
  previous line removes that corner.
* **Save** and **Load** buttons at the right edge of the map. The first Save asks for a name;
  after that Save writes the same save at once ("Saved: name"). Loading a save makes it the
  quick-save target; a new adventure starts without one. Back → Save still saves under a new
  name.
* Autosave: the game is saved as "autosave" whenever the app goes to the background (home
  button, screen off, another app), in addition to eZeus' yearly "autosave history".

## 0.1.11-dev
* Removed the house road check added in 0.1.10: it wrongly refused houses right beside a
  road. Houses can be placed anywhere again, as before.
* Checked: the adventure list shows all 24 adventures from your game folder – Zeus and
  Poseidon campaigns, Open Play maps and your custom adventures (see test report).

## 0.1.10-dev
* Desirability now works like the original game: every building spreads its value in
  square rings with the original steps (from your Model files), and houses take the best
  value under them. Houses evolve and devolve with the original thresholds for your
  difficulty (with the original "buffer" between devolving and evolving). This changes how
  attractive areas must be, so existing cities may settle at different house levels.
* (Removed again in 0.1.11) A house road check.
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
