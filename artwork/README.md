# Restored HD interface artwork

`hud/health-pack.png` is the new inventory icon, restored with the built-in image
generation tool from the medic's green pillbox model. `hud/health-pack.json`
records its reference identities, prompt and output hashes. Its 128px RGBA
companion is embedded in the shared Windows/Proton runtime and packed into the
existing UI atlas once, so it needs no optional texture pack or runtime decoder.
It is an added HUD element and remains available with HD artwork on or off.

These optional replacements were restored with the built-in image generation
tool from references supplied by the user's game installation. They preserve
the original composition and style, with newly reconstructed detail. They are
not original extracted textures, and they do not make the game playable without
the user's supported ISO. Original game designs belong to their respective
owners.

`hd-ui/manifest.json` is the shared build/package allowlist. Each record pins an
original texture identity and the approved replacement PNG by SHA256. Do not add
files directly to release folders. `provenance.json` records restoration prompts
and output hashes; private reference exports are deliberately excluded.

The runtime retains original dimensions and UVs. Transparent sprite content is
aligned to its authored bounds; menu photo alpha and locked-thumbnail coloring
remain original. Fonts, reticles and flat action icons are reconstructed locally
from the imported game. Soft health masks and animations keep their original
behavior. Missing or invalid replacements fall back safely.

The HD UI setting switches both mouse and controller interfaces. Artwork is
prepared at startup; there is no image generation or filtering during gameplay.

This candidate includes 75 restored PNGs covering 139 texture-name mappings.
Fonts, ammo digits, reticles and flat action symbols use local reconstruction.
The colored title logo, localized subtitle strips, and two previews with tiny
warning labels retain filtered original artwork where generated versions failed
visual review. Animated health masks retain their authored appearance.
