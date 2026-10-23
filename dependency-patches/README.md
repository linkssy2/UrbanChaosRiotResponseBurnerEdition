# Current and historical runtime changes

The current PC port uses the unmodified XboxXBERecomp checkout pinned in
`dependencies.lock.json`. Its integration changes are checked source
transformations in `scripts/prepare_xbe_*.py`, applied in the order recorded by
`scripts/build_xbe_game_trial.py` to private build output. No additional patch is
applied directly to the XboxXBERecomp checkout.

The maintained ADPCM decode-cache overlay selectively carries the two-block
optimization from upstream commit `1bb872b181c40de83ca2f69d9cd45b442a362d1d`,
with a local correction for cross-mix invalidation. Other audio output, DSP,
HRTF and routing changes in that upstream commit are not imported. The SDK pin
remains `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7`.

This includes the maintained rendering/performance, input/menu, HD UI,
viewmodel/scope, wall-depth, language and retained-object changes. Their production helpers and fixtures are in the source
snapshot; generated game output and original disc art are excluded.

`xboxrecomp.patch` and `../dependency-overrides/xboxrecomp/` preserve the previous
runtime work. Current preparation reuses selected keyboard/controller,
procedural prompt and shader-cache helpers from those overrides. Read
`docs/BUILDING.md` for the remaining private generated-input requirement.

The 0.07.5 Linux compatibility revision replaces Microsoft infinity literals with exact IEEE bit casts in the private shader preparation. See `scripts/test_xbe_shader_infinity.py` and `docs/LINUX-SHADER-COMPATIBILITY.md`. The SDK pin and shader cache validation are unchanged.
