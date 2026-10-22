# Original binaries and formats (reference edition)

Only hashes and header facts are recorded here; no executable bytes or game content.

## Executable

| Item | Value |
|---|---|
| File | `Zeus.exe` (GOG "Zeus and Poseidon", gameId 1207659039, English) |
| Size | 2 527 232 bytes |
| SHA-256 | `382bcd286192a06eb8fdc45f5c37390a19999f5db9c940c3ce2dfcd775e90a57` |
| Format | PE32, i386 (machine 0x14c), GUI subsystem, 4 sections (.text 1.9 MB, .rdata, .data 16 MB virtual, .rsrc) |
| Link timestamp | 2002-01-04 11:03:23 UTC |
| Version string | "Version 1.11" (embedded); `Zeus_MM.eng` "About" message: "Version 2.1" |
| Runtime deps | `mss32.dll` (Miles Sound System), `BINKW32.DLL` (Bink video), `ijl10.dll` (Intel JPEG) |
| Protection | none observed in strings (GOG build); not analysed further, not needed |

Digests of identifying data files are in `tools/assets/asset_manifest.json`.

## Data formats used by the port

| Format | Files | Reader | Notes |
|---|---|---|---|
| SG3 + 555 sprite banks | `DATA/*.sg3`, `DATA/*.555` | eZeus `textures/*`, `spriteData/*`, `offsets/*` | Impressions sprite index + 15-bit RLE pixels; read natively by eZeus. |
| ENG text / message tables | `Zeus_Text.eng`, `Zeus_MM.eng` | `platform/eengconverter.cpp` → XML → `exmlparser.cpp` | Layout from citybuilding-tools (MIT). Text: 16-byte name, 3×int32, 1000×(offset, count), data. Messages: 16-byte name, total/used int32, 80-byte entries, data. |
| PAK adventures | `Adventures/*.pak`, `Adventures/<name>/*.pak/.map/.set` | `engine/ecampaignreadpak.cpp` | Original campaign/mission definitions. |
| Original saves | `Save/*.sav`, `Adventures/tutorial*.sav` | not supported by eZeus | eZeus has its own save format. |
| eZeus `.e` packs | `interface.e`, `i15/30/45/60.e` | `ebinaryimageloader.cpp` + `esplitbinary.h` | Concatenated PNGs, offsets compiled into the engine. |
| eZeus saves | `eZeus/Save/<leader>/*` | `fileIO/` | int32 for all integers, 1-byte bool, IEEE float/double, little-endian. |

## Save compatibility policy

* Original Zeus `.sav` → eZeus/port: **not supported** (upstream limitation).
* eZeus desktop (x86-64) ↔ Android (arm64): expected compatible by construction (same
  serializer, fixed-width types, both little-endian). **Unverified on a device**; a
  cross-device round trip is a pending test (NEXT_STEPS.md).
* Saves are never written into the original installation; the importer never overwrites
  `eZeus/Save/`.
