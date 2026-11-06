# Urban Chaos: Riot Response — Burner Edition 0.14

Changes since the published 0.13 source snapshot:

- **Body dragging:** hold Interact while aiming at a nearby dead body's arm,
  leg, head or chest to grab it; hold again to release. The grab remains latched
  while moving, with a firmer pull and slight lift. Each co-op player owns their
  own grab, and ordinary short Interact presses retain their behavior.
- **Burner body shield:** an optional main-menu cheat replaces an acquired
  shield with an original-engine Burner ragdoll held at the chest. Other limbs
  remain dynamic. Blocked hits use flesh effects, and the normal shield's glass
  damage marks are hidden. Original shield progression and bash damage remain.
- **Shield-table pickups:** with the cheat enabled before the mission, the
  first level's two shield pickups appear as static seated Burners. The original
  pickup collision and scripts remain; equipped bodies use separate physics.
  All model resources are derived locally from the player's imported disc.
- **Controller deadzones:** separate per-player sliders for the two sticks,
  including an Original response option. Controller tuning is isolated from
  mouse/keyboard input, and controller-only settings stay on controller pages.
- **16:10 and settings organization:** added an explicit 16:10 aspect choice
  and 1280×800, 1920×1200 and 2560×1600 presets, with grouped Graphics and Controls
  lists. Existing profile IDs, bindings and FMV mode choices are preserved.

Windows and experimental Linux/Proton share the same runtime and asset recipes.
The existing **0.09 HD texture pack remains compatible**; this update requires
no new HD texture download. Saves, settings and installed add-ons are preserved.

Windows feedback confirmed controller deadzone isolation, the revised body-drag
response and native body-shield presentation in scoped candidates. Final static
pickup placement, head/chest grabbing and the later shield-bash correction need
their own live acceptance. Offline input, geometry, ownership and physics-hook
fixtures do not establish full-campaign or live Proton behavior. Co-op remains
experimental. This source snapshot is for inspection and contribution; a clean
bootstrap still requires qualified private title inputs as described in
`docs/BUILDING.md`.
