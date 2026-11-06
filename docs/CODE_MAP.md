# Burner Edition 0.14 source map

- `scripts/build_xbe_game_trial.py`: maintained build, preparation order and tests.
- `scripts/prepare_xbe_*.py`: pinned transformations of the XboxXBERecomp runtime
  and locally generated game code; edit these rather than staged build files.
- `scripts/xbe_*.cpp`, `.h` and `.inc`: title adapters, renderer integration,
  PC menus, input, split-screen, health packs and targeted fixes.
- `scripts/test_*` and check directories: contract/regression fixtures.
- `scripts/xbe_ragdoll_drag.*`: per-player held-Interact body dragging through
  the original input, collision query and physics-force paths.
- `scripts/xbe_corpse_shield*` and `scripts/corpse_shield_assets.py`: the optional
  native body-shield cheat and locally derived held/static pickup models.
- `scripts/xbe_controller_deadzone.h`: physical-stick response isolated from
  keyboard/mouse input; settings and 16:10 layout remain in the shared UI path.
- `native/`: only model, animation, texture and archive helpers reached by the
  current build/setup closure; unrelated historical reconstruction is excluded.
- `launcher/burner-xbe`: Windows launchers and common verified setup worker.
- `launcher/burner-linux`: Proton wrapper, native path tests and Steam guidance.
- `packaging/`: Spanish text, tool identity recipes and portable shader seed.
- `artwork/`: approved replacement UI artwork and provenance.
- `dependencies.lock.json` and `licenses/`: dependency identities and notices.

Game-specific addresses and verification boundaries are documented in
`docs/XBE-RUNTIME-MAINTENANCE.md`. Old runtime overlays and unrelated historical
source are deliberately absent from this snapshot.
See `docs/RAGDOLL-DRAG.md`, `docs/CORPSE-SHIELD.md` and
`docs/DISPLAY-AND-CONTROLLER-SETTINGS.md` for the new source paths, ownership
contracts and verification limits.
