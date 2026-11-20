# Burner Edition 0.21

- Address additional crash paths reported after v0.20 during the Angel Heights
  minigun sequence. Entity registration can grow beyond the original 1,024 slots,
  and the matching update, render, live-entity and pending-retirement lists grow
  with it. New characters receive valid IDs when the original storage is full.
- Preserve full-generation ID uniqueness, stable list-node addresses, original
  traversal order and cleanup through registration, retirement and world teardown.
  Native helper lookups use the extended registry too.
- Guard optional muzzle-flash creation and updates against missing or stale
  entities. Valid flashes continue to use the original creation and transform
  paths, including when extended entity storage is required.

Forever ragdolls remain available. Existing retention settings are unchanged;
this change does not evict retained bodies or promise unlimited memory.
The tester reported that turning Forever off prevented the crash. That supports
capacity pressure as a trigger; acceptance of the new fix with Forever enabled
remains pending.

The runtime passed 193 automated test steps, including a 3,075-entity combined
registration/cleanup test. Pool fixtures exercise 4,097 nodes, and the reproduced
character-attachment path succeeds with the original registry full. Four new
capacity fixtures also passed on native Linux. These bounded checks do not
establish a complete gameplay replay, long-session/save-load acceptance or live
Proton/Steam Deck compatibility.

Previous features and fixes remain included: lighting presets, both weapon
wheels, single-player wheel blur/slow time, the ember pointer, approved launcher
cover, optional HD-pack download/install, No HUD scope, dead-enemy blood pools
with ledge clipping, TV-audio correction and the shared co-op hostage fixes.
Co-op remains experimental. Manage > DLC / Prototypes remains visible but
unavailable; prototype integration is paused. No original game files or personal
state are bundled. The optional HD texture pack remains separate.

Windows and Linux/Proton share the same runtime and resources. Linux runs the
Windows executable through Proton. Full setup/update instructions remain
Markdown, with the existing TXT compatibility pointers preserved.

Source downloads are inspection/contribution snapshots. Clean bootstrapping
still requires the qualified title inputs described in [build status](BUILDING.md).
