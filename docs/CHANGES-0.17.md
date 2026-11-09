# Urban Chaos: Riot Response — Burner Edition 0.17

Changes since the published 0.16 source snapshot:

- **Frame ghosting:** add a separate Graphics option for previous-frame image
  trails. Gameplay blur continues to control scene-depth blur and co-op downed
  softening. Both settings apply immediately to single-player, both co-op views
  and shared in-game cutscene cameras. Disable both to suppress both effects.
- **Saved blur preferences:** when an older settings file has no Frame ghosting
  entry, inherit its Gameplay blur choice. An explicit new value takes priority.
  Mission transitions and authored effect events cannot override the final
  presentation setting. Effect timers and history rotation continue while Off;
  re-enabling does not revive expired effects or frozen frame history.
- **Weapon selector animation (#27):** the HUD now follows an accepted Previous
  Firearm selection backward instead of passing through unrelated weapon icons.
  Preserve native inventory eligibility, animation speed, fading and forward
  controller cycling. The pending direction belongs to the selecting inventory
  and target, survives key release, and is replaced by later forward or direct
  selection.

Retail-function fixtures cover blur/ghosting combinations, saved preferences,
effect lifecycle and rendering boundaries, along with carousel wraparound,
rapid changes, fading and co-op ownership. Windows user testing confirmed the
corrected carousel with three firearms and a larger ten-variant inventory in
an isolated test build. Its temporary weapon-grant cheat and test resources are
excluded from the regular runtime and this source snapshot. The release rebuild
and package checks are recorded separately from that live acceptance.

These controls do not remove glow, heat haze, authored particle streaks or blur
already encoded in prerendered movies. Exact reproduction of the reporter's
scenes and full-campaign coverage remain separate from the regression fixtures.
Linux/Proton gameplay remains unverified; Linux uses the Windows runtime through
Proton. Co-op remains experimental. Paused particle/lighting performance work
was not resumed for this update.

Both packages share the runtime and asset recipes. The existing **0.09 HD texture
pack remains compatible** and is unchanged. Clean source bootstrapping still
needs the qualified private title inputs described in `BUILDING.md`.
