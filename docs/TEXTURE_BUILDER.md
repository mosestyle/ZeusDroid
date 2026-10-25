# Texture builder

ZeusDroid can make its texture packs (`interface.e`, `i15.e`, `i30.e`, optionally
`i45.e` / `i60.e`) from the player's own original game files, so the eZeus texture
download is not needed.

## How it works

* `engine/texbuild/esg3file.*` reads the original sprite files `DATA/*.sg3` (index) and
  `DATA/*.555` (pixels). Written for this project from observations of the files
  (no code from other projects):
  * header 680 bytes (version at 4: 0xd5 = 64-byte image records, 0xd6 = 72-byte records
    with an alpha mask), 200 bitmap records of 200 bytes (image count at +124, first index
    at +128), then image records: offset, length, uncompressed length, mirror offset (+16),
    width/height (+20/+22), type (+50), top flag (+53), tile count (+55);
  * pixels are 15-bit (0RRRRRGGGGGBBBBB); green is expanded like a 6-bit value (g × 2)
    to match the existing packs;
  * plain images: raw pixels, 0xF81F transparent; sprites: run-length (255 n = skip n,
    c < 255 = c pixels); 0x7C00 (pure red) is shadow = black at alpha 127;
  * isometric images: 58 × 30 tiles in diamond rows at the bottom, then a run-length top;
  * mirror records: the referenced image flipped; alpha masks (0xd6) follow the colour
    data directly, run-length, 5-bit values.
* `engine/texbuild/etexmap.inc` (generated, indices only) says for every atlas frame which
  original image it is (file + record index + whether pure red means shadow). It was made
  once, locally, by matching the frames of the eZeus 0.8.2 pack against the decoded
  originals; the 30-px frames are the original images 1:1. Tools (`tools/texbuild/`):
  1. `texmap_gen.py` – first pass: one file + index base per atlas;
  2. `texmap_fix.py` – atlases not fully exact: tries every file / base (also bases
     derived from frame sizes) with and without shadow conversion, per-frame fallback;
  3. `texmap_scan.py` – frames still not exact: searches all records of fitting size
     (the original numbering has gaps in a few atlases, e.g. `stor_bays`);
  4. `texmap_emit.py` – writes `etexmap.inc` (also the list of full-screen pictures).
* Full-screen pictures (`<size>/Zeus_Data_Images/*.jpg`: menu, loading screens, maps of
  Greece, victory / defeat): 30 = the original `DATA/*.jpg` file byte for byte;
  15 / 45 / 60 = scaled to width × size / 30 (rounded down) and saved as JPEG (quality 92).
  All of them go into `interface.e`.
* `engine/texbuild/etexturebuilder.*` writes the atlas pages with the positions from
  `engine/spriteData/*` : 30 = original pixels, 15 / 45 / 60 scaled (`eresample`:
  area-correct triangle filter down, Catmull-Rom up, premultiplied alpha; interface
  atlases keep hard edges). Pages are stored as QOI images (`eqoi`), with an index file
  `<pack>.idx` (name position size); `eBinaryImageLoader` uses that index when present.
* On the first start without packs (`main.cpp`), the game shows "Preparing the game
  graphics…" with a progress bar and builds them once (desktop, 2 cores: about 16 s;
  `i15.e` + `i30.e` + `interface.e` ≈ 480 MB). `EZEUS_TEXBUILD_LARGE=1` also builds
  `i45.e` / `i60.e` (scaled up from the originals).
* Only the user's own game folder is read; nothing from the originals is stored in the
  repository (the map holds numbers only).

## Checks

* `tools/texbuild/compare_built.py <built> <eZeus pack>` compares frame by frame.
* Unit test `tests/native/test_qoi_resample.cpp`.
