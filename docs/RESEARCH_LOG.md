# Research log

Reverse engineering is used only for proven, specific questions (see AGENTS.md).

| # | Date | Question | Method | Result | Status |
|---|---|---|---|---|---|
| R1 | 2026-10-22 | Where do `Zeus_Text.xml` / `Zeus_MM.xml` come from and what exact format does eZeus need? | Unpacked `engconverter-0.5.exe` (UPX) and read strings: Qt4 "ENG Converter" by Impressions tools author → matched to MIT-licensed bvschaik/citybuilding-tools (`engconverter/src`). Read eZeus `exmlparser.cpp` (line-based). | Format reimplemented in Python and C++; byte-identical output on the user's files. No binary analysis of the game needed. | Done |
| R2 | 2026-10-22 | Does the game simulation depend on render FPS? | Source reading: `widgets/egamewidgetpaint.cpp` advances `incTime`/`incFrame` per paint; `emainwindow.cpp` paints at 20 FPS. | Introduced `eFrameClock` to keep 20 Hz ticks while rendering at 60 FPS. | Done (desktop verification pending, see TEST_REPORT) |
| R3 | 2026-10-22 | Is the eZeus save format portable between 64-bit x86 and arm64? | Source reading of `fileIO/ewritestream.h`. | All integers serialized as int32 via template; no pointers/size_t on disk. | Analysed; on-device test pending |

## Ghidra / REA status

Not installed yet: no open question so far required analysing `Zeus.exe`. REA's Windows
Ghidra provider is documented as experimental for 32-bit PE; when a parity question needs
binary evidence the plan is native Ghidra `analyzeHeadless` with project-owned scripts in
`tools/ghidra/` (Ghidra and its JDK come from GitHub releases, which are reachable from this
environment).
