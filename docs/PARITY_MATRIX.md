# Zeus + Poseidon parity matrix

Status values: `NOT_TESTED`, `MATCHED` (compared against the original PC game with
evidence), `PARTIAL`, `MISMATCH`, `BLOCKED`. "Runs in port" means the system works in the
eZeus engine as built here; it is **not** a parity claim.

Reference original: GOG Zeus + Poseidon, `Zeus.exe` "Version 1.11" (BINARY_COMPATIBILITY.md).
Engine: eZeus `ec765199` + this repo's platform changes (which do not touch game rules;
the only gameplay-visible change so far is the `q` key fix in text fields).

Phase F started 2026-10-24 with the cheapest strong evidence: the original game's own
balancing files in the user's install (`Model/Zeus_Model_<Difficulty>.txt`,
`Model/Tax_Sentiment_Model.txt`) and its text strings (`Zeus_Text.eng`). Tool:
`tools/parity/compare_model.py <game root>` writes a local report (`build/parity/`, not
committed because it contains original values). Behaviour comparisons that need the
original running (screen control on the user's PC, with permission) are still to do.

### Data parity against the original Model files (2026-10-24)

| Data | eZeus upstream vs original | Port action | Status |
|---|---|---|---|
| Building cost per difficulty | 341 of 540 building×difficulty values differ (eZeus uses base × one factor per difficulty, e.g. Hero ×2.25 where the original has individual values: road 3 vs 5, palace 250 vs 281, mint 200 vs 225; temples match) | Cost is read at run time from the user's `Zeus_Model_<difficulty>.txt` (`engine/engine/emodeldata.cpp`, hooked in `eDifficultyHelpers::buildingCost`) | `MATCHED` (by construction; road verified in-game: Hero 3 dr/tile) |
| Employees | 9 of 54 comparable differ: Sculpture Studio 12 vs 18; sanctuaries all 60 in eZeus vs 45–75 (Aphrodite, Ares, Atlas, Dionysus, Hades, Hera, Hermes, Poseidon) | Original value used when both give the building workers (`eEmployingBuilding`, also applied to loaded saves) | `MATCHED` (by construction) |
| Desirability value / range | 81 of 91 match; carrot and onion farm swapped; refinery range 6 vs 5; dock had none | Original values used, except agora and vendors (eZeus spreads the agora's desirability over its vendors) | `MATCHED` except agora model `PARTIAL` |
| Desirability spread (step, step size) | eZeus uses a smooth linear fall-off; the original uses steps | none yet (changes every house's evolution; needs side-by-side test) | `MISMATCH` (known) |
| House evolution desirability thresholds | eZeus uses its own scale (e.g. > 2, > 5, > 8) vs original 10/20/30/100 | none yet (tied to the spread model above) | `MISMATCH` (known) |
| House population capacity | common 8–60, elite 6–20 | – | `MATCHED` |
| Fire / damage risk increments | 158 / 115 of 540 differ (e.g. onion/carrot farms, agora vs vendors, piers) | none yet: eZeus applies risk differently for agora/vendors and docks; needs behaviour test | `MISMATCH` (known) |
| Tax sentiment table | identical for all 7 rates × 5 difficulties | – | `MATCHED` |
| Tax rates | "Very Low" and "Low" were swapped (wrong rate/sentiment pairing; panel cycled None, Low, Very Low, Normal) | fixed in `engine/engine/etaxrate.cpp` | `MATCHED` after fix |

### Claims from the Olympus fork

| Claim | Check | Result |
|---|---|---|
| f0c67c1 "god blessings match original zeus" (shorter lists of sanctifiable buildings per god) | Original `Zeus_Text.eng` has "is sanctified!" messages for University, Museum, Inventor's Workshop, Agora, Artisans' Guild – all removed or not listed by the Olympus change | Not ported: contradicts the original's own texts |
| Settlers arrive in groups of 4; house devolution differs | not yet checked | open |

| Area | Status | Evidence so far | Known differences / notes |
|---|---|---|---|
| Adventure list (Zeus + Poseidon) | NOT_TESTED (runs in port) | Desktop autotest `explore4`: list shows Sands of Betrayal, Founding of Athens (eZeus-made), Open Play ×3, Athens through the Ages, Life in the Mediterranean, Youngest Twins, Proetus and Bellerophon, Two Worlds Collide, Zeus and Europa, Peloponnesian War, Perseus and Medusa… | eZeus adds two own adventures (*Sands of Betrayal*, *Founding of Athens*). |
| Mission briefing / goals | NOT_TESTED (runs in port) | `explore5`: "Zeus and Europa – Founding of Thebes", goal "Population of 1000", original briefing text from Zeus_MM. | |
| Map loading, terrain | NOT_TESTED (runs in port) | `explore6`/`touch1`: Thebes map rendered with original terrain sprites | |
| Simulation clock / date | NOT_TESTED (runs in port) | `touch1`: date advanced 1 → 2 Jan within ~4 s of unpaused play | Upstream speed table `sSpeeds`; speed vs original not measured. |
| Scripted events (monsters) | NOT_TESTED (runs in port) | `touch1`: "Hydra in city" event message fired at game start of Founding of Thebes | Timing vs original unmeasured. |
| Pathfinding, walkers, delivery, staffing | NOT_TESTED | – | eZeus README: "new walker implementation" option exists. |
| Housing, desirability, services, hygiene | NOT_TESTED | – | Olympus fork claims upstream house devolution differs from original (candidate check). |
| Farms, industry, storage, trade, taxes | NOT_TESTED | – | |
| Gods, heroes, monsters, sanctuaries | NOT_TESTED | – | Olympus commit `f0c67c1` "god blessings match original zeus" – candidate check. |
| Immigration / population | NOT_TESTED | – | Olympus claims settlers should arrive in groups of 4 – candidate check. |
| Military, invasions, rival cities, colonies | NOT_TESTED | – | eZeus has AI opponent work on a separate branch (`ai_opponent`). |
| Campaign progression / victory / defeat | NOT_TESTED | – | |
| UI screens, overlays, messages | NOT_TESTED | Menus, roster, adventure list and in-game panel render with original art | Desktop default 1280×720; Android uses native resolution. |
| Hotkeys | N/A on touch | Back = Escape / cancel; touch bar `||` `-` speed `+` `R` `X` | Pause, speed and rotate have touch buttons (0.1.2+). |
| Camera zoom | PORT CHANGE (presentation only) | `phone_zoom_speed_lists.txt` | Original/eZeus: fixed texture sizes. Port adds 0.5×–3× world scaling (pinch, mouse wheel); 1× is pixel-identical to upstream. No effect on simulation or saves. |
| Music / sound | BLOCKED here | Audio files not copied to the test environment; headless test used a dummy audio driver | Needs a device test. |
| Speed control | NOT_TESTED | – | |
| Save / load | NOT_TESTED | Leader roster created and persisted (`eZeus/Save/moses`) | Original `.sav` files unsupported (upstream). |
| Frame-rate independence | PARTIAL | Design: 20 Hz ticks independent of 60 FPS rendering (`eFrameClock`); desktop measurement pending | |

## Discrepancy log

| ID | Scenario | Original | Port | Root cause | Fix / test | Status |
|---|---|---|---|---|---|---|
| D1 | Typing `q` in a text field (leader name, save name) | `q` is accepted | `q` ignored | `widgets/elineedit.cpp` scancode table omitted `SDL_SCANCODE_Q` | added case; covered by manual check | Fixed (not yet re-tested in-engine) |
