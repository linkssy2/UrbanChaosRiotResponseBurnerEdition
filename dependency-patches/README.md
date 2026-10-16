# Current and historical runtime changes

Version 0.03 uses the unmodified XboxXBERecomp checkout pinned in
`dependencies.lock.json`. Its local integration changes are reproducible source
transformations in `scripts/prepare_xbe_*.py`, applied in the order recorded by
`scripts/build_xbe_game_trial.py` to private build output. No additional patch is
applied directly to the XboxXBERecomp checkout.

`xboxrecomp.patch` and `../dependency-overrides/xboxrecomp/` preserve the previous
runtime work. Current 0.03 preparation still reuses the selected keyboard and
controller helpers, procedural prompt drawing and shader cache helpers in those
overrides. See `docs/BUILDING.md` for the remaining full-game bootstrap limitation.
