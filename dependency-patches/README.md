# Current and historical runtime changes

The current PC port uses the unmodified XboxXBERecomp checkout pinned in
`dependencies.lock.json`. Its integration changes are checked source
transformations in `scripts/prepare_xbe_*.py`, applied in the order recorded by
`scripts/build_xbe_game_trial.py` to private build output. No additional patch is
applied directly to the XboxXBERecomp checkout.

This includes the maintained rendering/performance, input/menu, HD UI and
crosshair changes. Their production helpers and fixtures are in the source
snapshot; generated game output and original disc art are excluded.

`xboxrecomp.patch` and `../dependency-overrides/xboxrecomp/` preserve the previous
runtime work. Current preparation reuses selected keyboard/controller,
procedural prompt and shader-cache helpers from those overrides. Read
`docs/BUILDING.md` for the remaining private generated-input requirement.
