# Agent / contributor rules

1. **Vanilla first.** Default behaviour must match the original Zeus + Poseidon. Touch and
   performance changes must not alter game rules; improvements are opt-in.
2. **Do not break the desktop build.** `cmake -S engine -B build/desktop -DEZEUS_SYSTEM_SDL=ON`
   must keep building; with `EZEUS_RENDER_FPS` unset the desktop loop is upstream-identical.
3. **No original game assets in git** (DATA, Audio, Adventures, .eng, .e packs, saves,
   screenshots of copyrighted art). `local-assets/`, `build/`, `dist/` are ignored. Only
   names/sizes/digests (`tools/assets/asset_manifest.json`) may be committed.
4. **Never modify the user's installation** (`C:\GOG Games\Zeus and Poseidon`); treat it as
   read-only source material.
5. **Evidence before claims.** Run real builds/tests and record commands, results and
   artifact hashes in `docs/TEST_REPORT.md`. Mark device behaviour "unverified" until a real
   device or emulator run exists. Never fabricate screenshots, FPS figures or test passes.
6. **No emulation layers.** No Box86/Box64/Wine/Winlator/streaming; the APK contains only
   native AArch64 code built from source.
7. **Platform code stays in `engine/platform/`**; no Android `#ifdef`s in simulation code.
8. **Reverse engineering is targeted:** only for a logged parity question
   (`docs/RESEARCH_LOG.md`), with project-owned scripts in `tools/ghidra/`; never publish
   decompiled game code.
9. **Keep `PROJECT_STATE.md` and `NEXT_STEPS.md` current** at every milestone.
10. Never replace a failing native build with a mock or a visual demo.
