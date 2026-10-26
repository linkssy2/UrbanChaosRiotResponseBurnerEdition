# 0.08 changes since 0.07

## Co-op and player settings

Experimental local split-screen co-op is available from the normal main menu through the same Play launcher as single-player. The current input pairing is keyboard/mouse for Player 1 and one controller for Player 2. Character selection uses rotating original 3D previews and the chosen character's body, arms and hands.

Each player has separate controls and world/weapon FOV. Accepted pause input records which player opened the menu, pauses the whole game, and gives that player the settings screen. Other graphics settings are shared. Apply/discard and saving preserve the other player's settings.

## Health, HUD and campaign cooperation

Stored health packs work in single-player and co-op. Each fresh level starts a player with one pack, with a maximum of three and further supply taken from medics' finite stock. The Use health pack action heals its owner or revives an eligible nearby partner. Old checkpoints without stored-pack data restore zero packs, rather than granting a new pack repeatedly.

Downed co-op players use a ragdoll-following first-person view, a ten-second countdown, pulsing red edges and blurred vision. Shared audio stays clear. Timeout or both players being downed fails the mission. Reviving restores the first-person weapon visibility, without giving the player a replacement inventory.

Co-op HUD elements are placed in their respective views. Shared objectives appear in both, while interaction/order hints, controller/keyboard glyphs, headshot and stun notices, pistol-spin readiness and item grants follow their owner. Finite pickup objects are consumed once. Evidence continues to contribute to shared mission progress. Additional work covers NPC orders, script proximity/progression, shared cinematic views, shield pickup/third-person animation, turret presentation, sound ownership, missing props and effects staying in their own view.

## Presentation and setup

- Improved T-Zero and Burner arm bends at wider FOV while preserving sleeve texture density.
- Reduced added darkening on Mission Failed, making the original death scene easier to see.
- Added proportional FMV framing: Fill crops to fill widescreen; 4:3 retains the complete picture. Bars baked into a movie can remain.
- Added local Spanish voice import from the supported European multilingual PS2 ISO. Bundled Spanish text and PC menus remain integrated. Game audio is converted on the user's PC and is not a downloadable release payload.
- Carried forward the Linux setup/symlink corrections and clearer Steam/Proton instructions from 0.07.5. Both platforms share the same runtime and asset recipes.
- Integrated selected upstream fixed-function missing-texture and consistent stereo audio-configuration corrections while retaining the qualified XboxXBERecomp SDK pin.

## Remaining limits

Co-op is experimental. A reported co-op aiming offset remains under investigation; this release does not claim it fixed. Full-campaign progression, helicopter sections, every character/action combination and live Proton co-op require more testing. The Angel Heights missing-officer report and exact-location HD wall-ledge visual confirmation remain open. HD HUD retains known prompt/display issues. No exhaustive campaign, language or fixed-frame-rate claim is made.

The existing 0.06 HD game texture ZIP is still compatible; it remains a separate optional download. No reinstall is required for an already valid installation. Source availability is not a claim that a clean checkout can rebuild the game: see [BUILDING.md](BUILDING.md).
