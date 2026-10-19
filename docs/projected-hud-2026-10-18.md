# Interaction brackets and widescreen HUD coordinates

The widescreen HUD expands its layout canvas from 640 to 853⅓ units, then
compresses queued X coordinates by ¾. This keeps ordinary health, ammunition,
and other edge-anchored widgets proportioned correctly at 16:9. It must not
compress positions that the camera has already projected onto the 640-unit
output viewport.

The retail interaction-bracket path is distinct:

- `73700` builds owner vtable `45312C` and ten bracket trees. `73500` builds each
  tree from four `HUD_Bracket_Quadrant.shader` sprites with corner anchors.
- Owner draw `75B90` enumerates `OPITEMS`. `47610` obtains the target's projected
  bounds from `47300`, which projects the eight corners through `470A0`.
- `470A0` divides clip coordinates by W and maps them through the actual camera
  viewport width and height. `75E1C–75E44` replaces the child descriptor's four
  bounds with that result, then `75E95` draws the bracket tree.

Applying the ordinary HUD correction again moved a centered projected position
from X=320 to X=240 (before the half-pixel convention), matching the visible
leftward displacement. `TrialProjectedHudScope` now excludes only this owner
while its bracket tree draws. It restores the surrounding HUD scope on return;
4:3 and ordinary HUD anchors retain their existing behavior. Camera FOV and HD
artwork do not need workarounds. HD replacement textures retain the original
guest texture dimensions and UV layout.

The regression test executes the original `470A0` projection for centered and
off-center points at four world FOVs, both aspect ratios, and three independent
weapon FOV settings. It verifies final queued bracket coordinates, unchanged
4:3 coordinates, nested scope restoration, and continued correction of static
HUD quads. Source preparation pins the original owner, draw, and projection
bytes before adding the scoped hook. No diagnostic output is added to the game.

The Orders wheel is unchanged: `71920` forwards its descriptor to `157670`, and
its entries use ordinary `79830` anchor wrappers. It does not use this projected
bound replacement path. Visual acceptance of the repaired interaction brackets
still requires an in-game comparison.
