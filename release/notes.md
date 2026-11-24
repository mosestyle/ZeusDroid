Native arm64 Android build of Zeus: Master of Olympus + Poseidon on the eZeus engine.

**You need your own copy of the game** (e.g. the GOG edition): install the APK, tap *Import game folder…*, pick your "Zeus and Poseidon" folder, then *Start game*. The first start prepares the graphics from your game files (about a minute, once). No game files are included.

### Changes in 0.1.26
* **Sanctuaries were invisible (all gods) – fixed.** Placing a sanctuary took the drachmas
  and marble but nothing appeared. The sanctuary layouts (where the stairs, statues, altar
  and temple go) were read from the eZeus data pack's `Sanctuaries/` folder, which the phone
  never has, so every sanctuary was placed with an empty layout. The layouts (from the eZeus
  source, GPL) are now built into the game. A save made after placing such an invisible
  sanctuary has no trace of it: load it in 0.1.26 and place the sanctuary again (the
  drachmas and marble paid then are not in the save, so they cannot be given back).
* **Artisans' Guild is in the Mythology menu**, below Sanctuaries, as in the original game
  (it was in the Industry menu). Artisans build the sanctuaries.
* **Cheats** (Menu → Cheats): *Delian Treasury* (+1000 drachmas, each tap), *Lots of Fishy*
  (unlimited food: houses stay fully stocked), *Fireballs from Heaven* (fireballs fall on
  your buildings and set them on fire), *Bowvine and Arrow* (towers shoot cows),
  *SoundFrags* (every death is heard, also off-screen), *Ambrosia* (win the scenario). The
  On/Off cheats last until the game is closed or another one is loaded; they are not saved.
  *Cheese Puff* (dairy workers in cheese suits) is not included: the cheese-suit graphics
  are not in the game files the app uses.
* **New signing key.** The key of earlier versions was lost; install 0.1.26 after
  uninstalling the old app (back up `Android/data/org.zeusport.game/files/ZeusPoseidon/eZeus/Save`
  first). The new key is kept on the developer's PC, so later updates install over it.

Earlier versions: docs/RELEASE_NOTES.md. APK sha256: `439807e95771a629aef7b6a1ec4586f1af80564a9af4d26cfea840375e3494ed`
