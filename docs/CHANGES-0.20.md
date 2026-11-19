# Burner Edition 0.20

- Repair empty entity-bucket unlink and reuse. The original defect could corrupt
  the update list, crash entity updates or leave scripted actors unticked. The
  regression reproduces the failure shape reported in GitHub issue #32; the
  reporter's exact complete gameplay/object history was not replayed.
- Keep authored co-op cinematic control and release on the intended player.
  Hostage targeting consistently considers a valid living co-op player.
- Share the original hostage slow-motion windows between both players,
  including the initial opportunity and the reload/headshot opportunity.
  Preserve native timing ramps, simulation clock and recovery.
- Correct Hope Street's narrow hostage tripwire admission and police-barrier
  progression so the partner can reach the rescued-hostage conversation.
- Lower and disable both players' weapons during an authored locked officer
  conversation. Keep companion movement available, suppress manual/automatic
  raising and queued wheel selections, then restore only equipment lowered by
  the conversation. Existing holsters and unrelated locks remain respected;
  already committed actions finish through their normal animation states.

All v0.19 features remain: Off/Low/Medium/High dynamic-lighting presets,
qualified rendering-work reductions, separate Guns and Melee / throwables
wheels, single-player wheel blur/slow time, the compact ember pointer, approved
launcher cover and optional HD-pack download/install. The No HUD scope,
dead-enemy blood pools with ledge clipping and TV electrocution audio fixes
remain included. Knocked-out enemies do not create blood pools.

Main missions use original spawn/checkpoint data, including Hope Street.
Private Hope Street and hotel replay spawns are excluded. Manage > DLC /
Prototypes remains visible but unavailable; prototype integration is paused.
No original/prototype game files, private test scenes or saves are bundled.
The optional 0.09 HD texture pack remains separate and needs no reinstall.

Windows and Linux/Proton use the same runtime and shared resources. Linux runs
the Windows runtime through Proton. Keep compatibility TXT pointers with the
packages; full setup/update instructions remain Markdown.

The user accepted the corrected Hope Street encounter on Windows, including
shared slow motion, barrier passage and both-player conversation weapon
suppression/restoration. Original-guest semantic fixtures, native Linux
fixture execution and package/updater checks are separate from live acceptance.
Other encounters, live Proton/Steam Deck and a full campaign pass remain
unverified. Co-op remains experimental; HD HUD remains a work in progress.

Source downloads are inspection/contribution snapshots. Clean bootstrapping
still requires the qualified title inputs described in [build status](BUILDING.md).
