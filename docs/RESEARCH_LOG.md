# Research log

Reverse engineering is used only for proven, specific questions (see AGENTS.md).

| # | Date | Question | Method | Result | Status |
|---|---|---|---|---|---|
| R1 | 2026-10-22 | Where do `Zeus_Text.xml` / `Zeus_MM.xml` come from and what exact format does eZeus need? | Unpacked `engconverter-0.5.exe` (UPX) and read strings: Qt4 "ENG Converter" by Impressions tools author → matched to MIT-licensed bvschaik/citybuilding-tools (`engconverter/src`). Read eZeus `exmlparser.cpp` (line-based). | Format reimplemented in Python and C++; byte-identical output on the user's files. No binary analysis of the game needed. | Done |
| R2 | 2026-10-22 | Does the game simulation depend on render FPS? | Source reading: `widgets/egamewidgetpaint.cpp` advances `incTime`/`incFrame` per paint; `emainwindow.cpp` paints at 20 FPS. | Introduced `eFrameClock` to keep 20 Hz ticks while rendering at 60 FPS. | Done (desktop verification pending, see TEST_REPORT) |
| R3 | 2026-10-22 | Is the eZeus save format portable between 64-bit x86 and arm64? | Source reading of `fileIO/ewritestream.h`. | All integers serialized as int32 via template; no pointers/size_t on disk. | Analysed; on-device test pending |

| R4 | 2026-10-24 | Do eZeus' balancing numbers match the original? | Parsed the user's `Model/Zeus_Model_*.txt` and `Tax_Sentiment_Model.txt`; parsed eZeus tables from source (`tools/parity/compare_model.py`). | Costs (most difficulties), some employees and 3 desirability entries differ; tax low/very-low swapped. Fixed by reading the original files at run time / swapping the two cases. Fire/damage and desirability spread left for behaviour tests. | Done (see PARITY_MATRIX) |
| R5 | 2026-10-24 | Is the Olympus fork's god-blessing list correct? | Searched the original strings (`Zeus_Text.eng`) for "sanctified". | Original has sanctification texts for buildings the fork removed → not ported. | Done |

## Ghidra / REA status

Not installed yet: no open question so far required analysing `Zeus.exe`. REA's Windows
Ghidra provider is documented as experimental for 32-bit PE; when a parity question needs
binary evidence the plan is native Ghidra `analyzeHeadless` with project-owned scripts in
`tools/ghidra/` (Ghidra and its JDK come from GitHub releases, which are reachable from this
environment).

## R6 – Desirability spread and house evolution in Zeus.exe (2026-10-24)

Targeted Ghidra work on the user's own GOG Zeus.exe 1.11 (scripts in `tools/ghidra/`,
decompiled output only in `build/ghidra-out/`, git-ignored, never committed). Findings are
described as rules; no decompiled code was copied.

* Model accessors: building getter (type, field) reads a 10-int-per-building table filled by
  the model loader; the house getter (level, field) reads a 21-int-per-level table.
* Desirability map: one signed byte per tile, cleared and rebuilt from zero. Every structure
  spreads its desirability in square rings around its footprint, distance 1..min(range, 6),
  never on the footprint itself. The value starts at "initial desirability" and the step size
  is added after every `step` rings. Each addition is clamped to -100..100. Large or
  non-square structures (sanctuaries, docks, horse ranch, agora...) go through a rectangle
  variant whose step counter starts at 1 (the first change comes one ring earlier).
  Avenues and boulevards spread per tile; other chained structures (roads, walls) nothing.
* House desirability = the highest map value under the footprint (+10 under one flag not
  identified yet).
* Evolution check (returns devolve / stay / evolve): devolve when DES ≤ the level's
  "devolve" value, or a need of the current level is missing; otherwise evolve when DES ≥ the
  level's "evolve" value and every need of the next level is met. House row columns: 0
  devolve, 1 evolve, 2 culture, 3 water, 4 stadium/museum, 5 soldier %, 6 max horses,
  7 horses, 8 food types, 9 fleece, 10 oil, 11 wine, 12 arms, 13 max arms, 14/15 crime,
  17 capacity, 18 tax multiplier, 20 disease (the file's own header numbers are off by one
  after column 4).
* A tile-category helper (likely the paved-road look) is "DES > 4"; the desirability overlay
  uses four bands (< 0, < 16, < 30, ≥ 30).

Port: implemented in `eHeatMap` (step mode), `eHeatGetters::appeal/eliteAppeal`,
`eBuilding::appeal` (max) and `eSmallHouse/eEliteHousing::updateLevel/missing`, all reading
the user's Model files at run time; disabled together with the rest of the model data by
`EZEUS_MODEL_DATA=0` (then eZeus' original behaviour is used).
