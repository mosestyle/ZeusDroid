Native arm64 Android build of Zeus: Master of Olympus + Poseidon on the eZeus engine.

**You need your own copy of the game** (e.g. the GOG edition): install the APK, tap *Import game folder…*, pick your "Zeus and Poseidon" folder, then *Start game*. The first start prepares the graphics from your game files (about a minute, once). No game files are included.

### Changes in 0.1.29
* **0.1.28 crashed at start – fixed.** The new logo code freed a picture twice (the
  engine's texture loader already frees it); the phone's memory allocator stops the app
  on that. The logos stay.

Earlier versions: docs/RELEASE_NOTES.md. APK sha256: `549e2f2c3d1237eaca5309934303b45d5ab5801993f45d2e9f951a7e9839e498`
