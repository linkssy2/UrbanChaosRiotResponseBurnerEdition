# Current and historical runtime changes

The current 0.08 port uses the unmodified XboxXBERecomp checkout pinned to
`c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7` in `dependencies.lock.json`.
Checked `scripts/prepare_xbe_*.py` transformations stage local overlays in the
order given by `scripts/build_xbe_game_trial.py`. They do not modify the SDK checkout.

Three selected upstream changes are integrated:

- `1bb872b181c40de83ca2f69d9cd45b442a362d1d`: the two-block ADPCM decode cache,
  with local cross-mix invalidation correction, through `prepare_xbe_audio_decode_cache.py`.
  This does not import the commit's other DSP, HRTF or audio-routing changes.
- `c9cda4ac39e1e8823a1517f950966a7ee1bd3905`: fixed-function missing-texture
  inputs preserve incoming color, through `prepare_xbe_missing_texture.py`.
  Bound textures and programmable paths preserve their sampling behavior.
- `5f760a2714cb1abe11b8620354246d0a659ac132`: consistent stereo reporting
  in direct audio and EEPROM queries, through `prepare_xbe_audio_config.py`.

These scoped backports do not mean all later upstream changes were adopted.
The source snapshot also contains maintained rendering, input, menus, HD UI,
FOV, language and co-op overlays with their regression fixtures. Original game
resources and generated title code remain excluded.

`xboxrecomp.patch` and `../dependency-overrides/xboxrecomp/` preserve earlier
runtime work. Current preparation reuses selected input, procedural prompt and
shader-cache helpers; the import worker also retains XDVDFS support. They do not
replace the current SDK. Retain all dependency notices and see
`docs/BUILDING.md` for qualified private input requirements.
