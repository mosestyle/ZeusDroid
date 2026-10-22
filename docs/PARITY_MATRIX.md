# Zeus + Poseidon parity matrix

Status values: `NOT_TESTED`, `MATCHED` (compared against the original PC game with
evidence), `PARTIAL`, `MISMATCH`, `BLOCKED`. "Runs in port" means the system works in the
eZeus engine as built here; it is **not** a parity claim.

Reference original: GOG Zeus + Poseidon, `Zeus.exe` "Version 1.11" (BINARY_COMPATIBILITY.md).
Engine: eZeus `ec765199` + this repo's platform changes (which do not touch game rules;
the only gameplay-visible change so far is the `q` key fix in text fields).

No side-by-side comparison with the original game has been run yet, so nothing is `MATCHED`.
Comparison requires running the original on the PC (possible via the linked computer's
screen-control tools with the user's permission) or Ghidra evidence; see NEXT_STEPS.md.

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
| Hotkeys | N/A on touch | Back = Escape | `P` (pause) has no touch equivalent yet – **open item**. |
| Music / sound | BLOCKED here | Audio files not copied to the test environment; headless test used a dummy audio driver | Needs a device test. |
| Speed control | NOT_TESTED | – | |
| Save / load | NOT_TESTED | Leader roster created and persisted (`eZeus/Save/moses`) | Original `.sav` files unsupported (upstream). |
| Frame-rate independence | PARTIAL | Design: 20 Hz ticks independent of 60 FPS rendering (`eFrameClock`); desktop measurement pending | |

## Discrepancy log

| ID | Scenario | Original | Port | Root cause | Fix / test | Status |
|---|---|---|---|---|---|---|
| D1 | Typing `q` in a text field (leader name, save name) | `q` is accepted | `q` ignored | `widgets/elineedit.cpp` scancode table omitted `SDL_SCANCODE_Q` | added case; covered by manual check | Fixed (not yet re-tested in-engine) |
