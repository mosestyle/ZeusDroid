# Licence notice

This project ("ZeusPoseidonAndroid", an Android port of eZeus) is distributed under the
**GNU General Public License v3.0** (see `eZeus-GPL-3.0.md`), because it is a derivative
of eZeus (c) Maurycy Liebner and contributors. Anyone who receives the APK is entitled to
the complete corresponding source code (this repository at the matching commit).

Bundled third-party components (compiled into the APK):

| Component | Version | Licence | File |
|---|---|---|---|
| eZeus | ec765199 (+ changes in this repo) | GPL-3.0 | eZeus-GPL-3.0.md |
| SDL2 (C library + Android Java glue) | 2.32.10 | zlib | SDL2-zlib.txt |
| SDL2_image (stb_image backend) | 2.8.12 | zlib (stb: public domain/MIT) | SDL2_image-zlib.txt |
| SDL2_mixer (minimp3, dr_flac, stb_vorbis, built-in WAV) | 2.8.2 | zlib (minimp3: CC0) | SDL2_mixer-zlib.txt, minimp3-CC0.txt |
| SDL2_ttf with vendored FreeType | 2.24.0 | zlib; FreeType: FTL (chosen) | SDL2_ttf-zlib.txt, freetype-FTL.txt |

Tools / reimplemented knowledge (not bundled as code):

| Component | Licence | Use |
|---|---|---|
| citybuilding-tools engconverter (Bianca van Schaik) | MIT | .eng format reimplemented in `tools/assets/eng_to_xml.py` and `engine/platform/eengconverter.cpp` |
| Augustus (Caesar III) | AGPL-3.0 | not used; no code copied |
| Olympus (eZeus fork) | GPL-3.0 | compared only; no code merged yet |

**Not included and not licensed by this project:** Zeus: Master of Olympus and Poseidon
(art, sounds, music, texts, maps, executables) are copyrighted by their owners. The eZeus
`.e` texture packs are derived from that art and are obtained by the user from the eZeus
release page. Neither is part of the APK or of this repository.
