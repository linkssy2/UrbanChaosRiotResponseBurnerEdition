# Urban Chaos: Riot Response - Burner Edition

**PC port - 0.08**

An unofficial PC port powered by [XboxXBERecomp by TGP482](https://github.com/TGP482/XboxXBERecomp), with Direct3D 12 rendering and statically recompiled original Xbox gameplay. Earlier Burner Edition work and retained helpers use [xboxrecomp by sp00nz](https://github.com/sp00nznet/xboxrecomp).

## Changes since 0.07

- Experimental local split-screen co-op from the normal main menu, rotating character previews, and character-specific bodies and first-person arms.
- Separate player controls and world/weapon FOV. Pausing stops both players; the player who pauses owns the settings screen.
- Stored health packs in single-player and co-op, medic resupply, partner revival, and a ten-second downed timer with pulsing red edges and blurred vision.
- Co-op ownership fixes for HUD, contextual prompts, personal combat rewards and pickups, plus shared objectives, campaign events and cinematic presentation.
- Revival, missing sound/prop, shield/weapon animation and cross-viewport effects fixes; improved arm bends and sleeve mapping at wider FOV.
- More visible death scenes, proportional FMV Fill/4:3 modes, and optional Spanish voices extracted locally from a supported PS2 ISO.
- Linux setup and path fixes carried forward from the compatibility release, and selected upstream missing-texture/stereo audio corrections.

See [changes and testing limits](docs/CHANGES-0.08.md). Co-op is experimental; the reported co-op aiming offset and full-campaign acceptance remain open. HD HUD still has known prompt/display issues; the original HUD remains available with HD HUD and menus Off.

## Install and play

1. Extract the matching Windows x64 package to a writable folder.
2. Run **Burner Edition Setup.exe** and select your supported original Xbox ISO or XISO.
3. Use **Play Burner Edition.exe**. Choose **Single player** or **Co-op** from the same main menu.

No separate co-op shortcut, executable or private launch flag is needed. Co-op currently uses keyboard/mouse for Player 1 (upper view) and one controller for Player 2 (lower view). Select characters, mark both Ready, then select a campaign level. Assign Player 2's Use health pack binding in Controls before relying on revival.

Windows requires Windows 10/11 x64 and a Direct3D 12-capable GPU. Original disc files, Xbox executables, game audio, movies, fonts and saves are not distributed. Setup derives required resources from the user's imported game. Approved restored HD UI replacement artwork and the custom Burner Edition cover/icon are included; see `artwork/README.md`.

Settings and saves are portable under `GameData/xbe-runtime`; profiles are inside `hdd`. For an update, extract into a new folder and **copy** your existing `GameData` beside the new launchers. Keep a backup. Existing portable data takes priority over first-import migration from the old AppData installation. Play uses portable data and does not fall back to AppData. The original ISO is unnecessary after successful import.

## Languages and optional Spanish voices

Select a language in Setup or **Options > Language**, then restart. The supported Xbox disc supplies English, French, Italian and German. ZARF's Spanish text, supplemental translations and PC menus are bundled. The launcher prepares translated game files locally on first Play after import or update; no separate Spanish text download is needed.

Spanish voices are optional. Use Setup's Spanish-voice import with your own supported European multilingual PS2 ISO (SLES-53991, En/Fr/De/Es/It). Extract the ISO first if it is inside a ZIP. The launcher converts dialogue and cutscene audio locally, leaving other languages and your original Xbox import intact. Without this import, Spanish uses English voices. No Spanish game-audio archive is provided for download. Translation and voice coverage still need community review.

## Linux through Proton

The experimental Linux package uses the **same Windows runtime**, release version and asset recipes through user-supplied Steam/Proton. It is not a native Linux port. Follow [the Linux instructions](launcher/burner-linux/README-linux.txt) for the recommended Steam setup and normal worker launch command, including local ISO/voice import.

A Linux tester confirmed setup and gameplay with the earlier compatibility build. That does not verify 0.08's new features, co-op, every GPU, or Steam Deck. Offline launcher checks and Windows success are not live Proton acceptance.

## Optional HD game textures and shader seed

HD game textures remain a separate download credited to **xXthe.RockoXx**, creator of the original PS2 U.C.riot.v.1.2.HD.textures pack. The unchanged 0.06 pack is compatible with 0.08. Install its ZIP through Setup, restart, then enable **Graphics > HD game textures**. Existing valid installations do not need reinstalling. This setting is separate from HD HUD.

The bundled portable shader seed prepares observed shader variants and preserves learned caches. It contains no GPU-driver pipeline cache. New variants and other first-use work can still cause stalls; a fixed campaign-wide frame rate is not promised.

## Source and verification

See [the code map](docs/CODE_MAP.md), [runtime maintenance](docs/XBE-RUNTIME-MAINTENANCE.md), [source/build status](docs/BUILDING.md), and [credits](CREDITS.md). This archive is an inspection and contribution snapshot. A clean checkout cannot yet reproduce the complete game without qualified private generated title inputs. It includes no such generated game code or original disc payloads.

Keep offline semantic tests, production compilation, and live visual/audio/input acceptance separate. The Angel Heights missing-officer report remains unresolved; the HD wall-ledge repair lacks confirmation at the reporter's exact location. Co-op campaign coverage, helicopter sequences, all character/action combinations and Proton co-op need further gameplay testing. Switch 2 USB hardware support remains experimental; Bluetooth is unsupported. Normal release tracing is disabled; failures can produce bounded error reports.
