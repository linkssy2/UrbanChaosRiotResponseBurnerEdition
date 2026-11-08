# Urban Chaos: Riot Response — Burner Edition 0.16

Changes since the published 0.15 source snapshot:

- **Firearm cycling (#27):** keep the sampled Next/Previous direction with the
  guest input publication. Background controller-discovery polls can no longer
  reset a queued Previous press into a forward step. The original firearm
  eligibility and selection loop remains in use, including rebound keys.
- **Optional controller and mouse input (#6):** add a saved, default-Off
  single-player Controls setting. Retain native stick look and apply raw mouse
  once afterward. Physical deadzones exclude keyboard-owned axes, separate
  Interact/Reload behavior remains, and relevant spin/conversation inputs accept
  their appropriate source. Menus keep last-active-device navigation; co-op
  keeps separate device ownership.
- **Settings layout:** reserve help-text space inside Graphics and Audio frames,
  moving the Graphics scroll area and separator with it. English/Spanish and
  original/HD font bounds are covered at 4:3, 16:10, 16:9, 21:9 and 32:9.

The production host/retail firearm regression passes 20 three-/four-weapon
scenarios, including wheel and rebound-key inputs with interleaved controller
polls. The preceding implementation fails six of those cases. Windows live
testing confirmed two-way pistol/shotgun switching and settings descriptions,
scrolling, right-click Back, and the new toggle's default/Apply/Discard behavior.
Those live checks used the preceding private candidate; the final release
rebuild is checked separately and has not received a new gameplay run.

Live cycling with three or more weapons, simultaneous physical controller/mouse
input, and Linux/Proton gameplay remain unverified. The user accepted the cycling
candidate for community testing; this does not turn those gaps into live passes.
Co-op remains experimental. The paused explosion/particle performance work was
not resumed for this update.

Both packages share the runtime and asset recipes. The existing **0.09 HD texture
pack remains compatible** and is unchanged. Clean source bootstrapping still
needs the qualified private title inputs described in `BUILDING.md`.
