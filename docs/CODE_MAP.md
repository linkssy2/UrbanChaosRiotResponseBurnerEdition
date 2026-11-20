# Burner Edition 0.21 source map

- `scripts/build_xbe_game_trial.py`: maintained build, preparation order and tests.
- `scripts/prepare_xbe_*.py`: pinned transformations of the XboxXBERecomp runtime
  and locally generated game code; edit these rather than staged build files.
- `scripts/xbe_*.cpp`, `.h` and `.inc`: title adapters, renderer integration,
  PC menus, input, split-screen, health packs and targeted fixes.
- `scripts/test_*` and check directories: contract/regression fixtures.
- `scripts/xbe_trial_menu_input.cpp`, `xbe_controls*` and
  `xbe_keyboard_bindings.inc`: published firearm direction, optional mixed
  controller/mouse gameplay and separate co-op ownership.
- `scripts/xbe_firearm_cycle.h` and `prepare_xbe_firearm_cycle.py`: original
  inventory cycling and the matching reverse HUD carousel, scoped to the
  selecting player and target. The pickup fixture shares the selection hook.
- `scripts/xbe_ragdoll_drag.*`: per-player held-Interact body dragging through
  the original input, collision query and physics-force paths.
- `scripts/xbe_corpse_shield*` and `scripts/corpse_shield_assets.py`: the optional
  native body-shield cheat and locally derived held/static pickup models.
- `scripts/xbe_controller_deadzone.h`: physical-stick response isolated from
  keyboard/mouse input; settings and 16:10 layout remain in the shared UI path.
- `scripts/prepare_xbe_simple_state_costs.py`, `prepare_xbe_lighting_bounds.py` and
  `prepare_xbe_sync_downsample_costs.py`: qualified render-state, lighting bounds
  and readback optimizations, with production-derived comparison fixtures.
- `scripts/xbe_effect_settings.*` and `prepare_xbe_effect_settings.py`: Off/Low/
  Medium/High dynamic-lighting coverage policy and native-pass gating. Particle
  density is unchanged; the separate lighting-resolution option was removed.
- `scripts/prepare_xbe_vertex_costs.py` and `xbe_packed_vertices.inc`: exact packed
  vertex conversion and eligible smooth-strip preservation, with retained list
  fallbacks and production-derived equivalence fixtures.
- `scripts/xbe_receiver_inverse.h` and `prepare_xbe_receiver_inverse.py`: exact
  inverse reuse only at the qualified receiver call, preserving other callers.
- `scripts/xbe_dirty_epoch.h` and `prepare_xbe_dirty_epoch.py`: reuse unchanged
  texture dirty epochs with original map ownership and write invalidation.
- `scripts/xbe_weapon_wheel.*` and `prepare_xbe_weapon_wheel.py`: owned-inventory
  snapshots, native weapon eligibility/selection and separate Guns versus
  Melee/throwables groups. Host input latches wheel kind and source per player;
  overlap, handoff, menus and invalid actors cancel rather than equip stale data.
- `scripts/xbe_weapon_wheel_ui.*`, `xbe_frontend_fonts.*` and `xbe_hd_ui.*`:
  viewport-clipped radial UI and existing original/restored HUD portraits in the
  shared font atlas. The renderer reads copied snapshots, not guest pointers.
- `scripts/xbe_weapon_wheel_time.*`, `xbe_weapon_wheel_blur.h` and their preparers:
  qualified solo simulation at 20% speed and final-presentation background blur
  while choosing. Co-op wheels add no time scaling; authored hostage slow motion
  is shared by both players. Menu/audio/input clocks are unchanged.
- `native/runtime/menu_cursor.h` and `scripts/prepare_xbe_cursor.py`: procedural
  ember arrow, DPI-size cache, tip hotspot and window-owned cursor visibility.
- `scripts/xbe_audio_dsp.inc` and `xbe_audio_filter.h`: environmental sends and
  per-voice filtering connected to the pinned runtime's audio processors.
- `scripts/xbe_graffiti_alpha.h`: exact-identity graffiti opacity enhancement.
- `scripts/xbe_graphics_settings.*`, `prepare_xbe_coop_aim_blur.py` and
  `xbe_coop_downed_blur.cpp`: separate shared Gameplay blur and Frame ghosting
  options, saved-setting migration and their final presentation consumers.
- `native/`: only model, animation, texture and archive helpers reached by the
  current build/setup closure; unrelated historical reconstruction is excluded.
- `launcher/burner-xbe`: Windows launchers and common verified setup worker.
- `launcher/burner-linux`: Proton wrapper, native path tests and Steam guidance.
- `packaging/`: Spanish text, tool identity recipes and portable shader seed.
- `artwork/`: approved replacement UI artwork and provenance.
- `.github/ISSUE_TEMPLATE/`: the current bug and performance report template.
- `dependencies.lock.json` and `licenses/`: dependency identities and notices.

Game-specific addresses and verification boundaries are documented in
`docs/XBE-RUNTIME-MAINTENANCE.md`. Old runtime overlays and unrelated historical
source are deliberately absent from this snapshot.
See `docs/RAGDOLL-DRAG.md`, `docs/CORPSE-SHIELD.md` and
`docs/DISPLAY-AND-CONTROLLER-SETTINGS.md` for the new source paths, ownership
contracts and verification limits.

- `scripts/xbe_blood_clip.h` and `xbe_blood_pools.cpp`: creation-time support sampling and reusable per-draw clipping scratch. Flat supported pools retain the original draw path.
- `scripts/xbe_tv_audio.h`: exact bank/sample/command matching applies the accepted scene correction once during loading.

- `launcher/burner-xbe/world_textures.py`: hash/size-pinned official pack download
  through the existing bounded HTTPS helper, followed by verified installation.
- `launcher/burner-xbe/import_game.py`: `download-textures` worker action;
  `Launcher.cs`: Manage > Add-ons download/install button.
- `launcher/burner-xbe/test_texture_download.py`: download, cancellation, failure
  cleanup, installation, and preservation regression checks.

- `scripts/prepare_xbe_entity_records.py` and `test_xbe_entity_records.cpp`:
  repair empty-bucket unlink/reuse and retain the original failure witness.
- `scripts/xbe_coop_cinematics.*`: keep authored cinematic control and release
  on the intended player without replacing the campaign-wide player manager.
- `scripts/xbe_coop_hostage.*`: select a valid living co-op target consistently
  through hostage aim, facing and proximity behavior.
- `scripts/xbe_coop_hostage_time.*`: admit either living player through the
  original hostage time-control path; both players use the original shared
  world clock and view feedback. Native ramp and restoration remain intact.
- `scripts/xbe_coop_progression.*` and `coop_progression_assets.py`: verified
  authored trigger census, Hope Street tripwire admission and police-barrier
  progression guards.
- `scripts/xbe_coop_interactions.*`: owned conversation admission and control
  release, both-player weapon lowering/suppression, and native restoration.
  The companion's movement remains available; already committed actions finish
  through native animation state machines. The weapon wheel checks the same
  conversation predicate before opening or consuming a queued selection.

- `scripts/xbe_entity_registry.h` and `prepare_xbe_entity_registry.py`: extend
  full-generation entity handles after the original inline table fills; cover
  generated lookup, retirement/reset paths and native helper consumers. Original
  inline IDs remain intact. `xbe_entity_registry_pins.json` binds emitted bodies.
- `scripts/xbe_entity_pool.h` and `prepare_xbe_entity_pool.py`: add stable slabs
  to the four current-world node pools only when exhausted, and release them
  through the original guest allocator/free lifecycle. Existing nodes and order
  remain intact; guest calls preserve the full CPU context and MXCSR.
- `scripts/xbe_entity_parent.h` and `prepare_xbe_entity_parent.py`: validate
  optional muzzle-flash owner identity and handle missing flash transforms in
  both generated player-update entries.
- `scripts/prepare_xbe_entity_capacity.py`: extract final emitted routines into
  `test_entity_capacity` and `test_entity_body`. Together with registry, pool and
  parent fixtures, these cover overflow IDs, generation reuse, combined world
  list ownership, retirement and the reported character-attachment failure.
