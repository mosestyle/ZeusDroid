Native arm64 Android build of Zeus: Master of Olympus + Poseidon on the eZeus engine.

**You need your own copy of the game** (e.g. the GOG edition): install the APK, tap *Import game folder…*, pick your "Zeus and Poseidon" folder, then *Start game*. The first start prepares the graphics from your game files (about a minute, once). No game files are included.

### Changes in 0.1.25
* **Thin black lines over buttons, panels and dialogs (Medium) fixed.** The interface is
  made of many small picture pieces; with Medium each piece was scaled on its own, which
  left thin seams between them on the phone's graphics chip. Now the whole frame is drawn at
  full size into one picture first, and that picture is scaled to the screen in one step.
* Options on the phone: the texture size tick boxes are gone (the zoom graphics are always
  made from your game files; the larger sets would only be enlarged copies). The importer
  no longer lists "High zoom textures".

Earlier versions: docs/RELEASE_NOTES.md. APK sha256: `10c5352c638689910ef23d2f6de49000679e0141d0fc911a24996493d12cd4a7`
