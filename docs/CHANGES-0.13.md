# Urban Chaos: Riot Response — Burner Edition 0.13

- **Launcher refresh:** game-inspired styling, organized Manage sections, matching button sizes and uncropped cover artwork. Existing launcher updates install the new design.
- **Skip Startup:** optional launcher toggle skips intro videos and Press Start straight to profile selection. Mission videos still play normally.
- **Game icon:** the running game now uses the Burner Edition icon in the taskbar.
- **Movement inertia:** new per-player Controls toggle removes movement input buildup when Off, without increasing movement speed. On keeps the original behavior.
- **Death convulsions:** brief, varied whole-body bursts for killed enemies and downed co-op players, including head movement. Toggle them in Graphics.
- **Persistent corpses:** fixed regional cleanup removing bodies despite Ragdoll amount being set to Forever.
- **FMV Fill stretch:** fills the screen by removing verified black borders and stretching the remaining picture without cropping scene content. Existing Fill and Original aspect ratio remain available.
- **HUD and audio:** fixed the health-upgrade marker appearing before the rest of the HUD. The Audio menu now clearly states that Stereo/5.1/7.1 changes apply immediately.
- **Controller Info:** hides the health-pack HUD while viewing the controls diagram and supports right-click to go Back.
- **Helper tips:** corrected widescreen placement of heal, orders and other gameplay hints, keeping their button symbols together in single-player and co-op.
- **Co-op aiming blur:** isolated each player's aiming effect and its image samples to their own view, including at different render scales.
- **Co-op hazards and prompts:** corrected fire/electrical hazards skipping Player 1 and checking Player 2 twice, and cleared stale "Waiting for partner" messages without bypassing mission checkpoints.
- **Canals railing:** restored the reverse side of the bridge's road-edge metal strips during the opening cutscene.
- **Code review fixes:** independent co-op health-bar animation, complete Walk/HUD control defaults, safer crash reporting, and removal of unused rendering and preparation code.

Windows and experimental Linux/Proton share the same runtime. The **0.09 HD texture pack remains compatible**; no new download or reinstall is needed. Updates preserve saves, settings and installed add-ons.

Windows user testing confirmed the enemy convulsions/toggle, reported corpse-retention area, Fill stretch, startup skipping, HUD fix, helper-tip placement, co-op aiming-blur isolation, fire damage for both players and cleared waiting messages. Movement inertia, downed co-op convulsions, the latest railing correction, direct profile startup, taskbar icon and this update's Linux/Proton gameplay still need live testing. Co-op remains experimental; HD HUD prompting still needs work, and the original HUD remains available.
