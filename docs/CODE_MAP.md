# Burner Edition 0.17 source map

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
