Urban Chaos: Riot Response - Burner Edition 0.08

HD HUD is still a work in progress and has known prompt/display issues.
Turn HD UI Off to use the original HUD, which works normally.

PC PORT
Windows x64 PC port using XboxXBERecomp by TGP482, with native Direct3D12 rendering,
PC controls and menus. Original gameplay is statically recompiled from Xbox code.
XboxXBERecomp: https://github.com/TGP482/XboxXBERecomp

NEW IN 0.08 (SINCE 0.07)
- Experimental local split-screen co-op from the normal main menu, with rotating character previews and character-specific bodies and first-person arms.
- Independent player controls and world/weapon FOV. The player who pauses owns the settings screen; both players pause together.
- Stored health packs in single-player and co-op, medic resupply, partner revival and a ten-second downed timer with pulsing red edges and blurred vision.
- Co-op HUD/prompt ownership, shared objective notifications, personal headshot/stun/spin rewards, pickups, scripted progression and shared cinematic presentation.
- Co-op fixes for revival crashes and hidden weapons, missing sounds/props, shield and weapon animations, and effects crossing between views.
- More natural elbow/shoulder behavior for T-Zero and Burner first-person arms at wider FOV, preserving texture density.
- Less added darkening on Mission Failed so the original death scene is easier to see.
- Proportional FMV framing: Fill crops to fill the widescreen view; 4:3 preserves the complete image.
- Optional Spanish voices imported locally from your European PS2 ISO. Spanish text remains included; no game-audio download is distributed.
- Carries forward the Linux setup/symlink fixes and clearer Steam instructions from the 0.07.5 compatibility work.
- Selected XboxXBERecomp fixes for missing-texture behavior and consistent stereo audio configuration.

Co-op is experimental. Full-campaign progression, all character/action combinations,
helicopter sections and Proton co-op still need gameplay testing. Investigation of
a reported co-op aiming offset remains open.

INSTALL
Linux package users: follow README-linux.txt and its recommended Steam setup.
The steps below apply to the Windows package.
1. Extract the entire ZIP to a writable folder (not Program Files or inside ZIP).
2. Run Burner Edition Setup.exe and choose your supported original Xbox ISO.
   Both the verified full-disc ISO and extracted .xiso.iso layouts are accepted.
3. Once imported, use Play Burner Edition.exe to play without opening setup.
   Keep the EXEs, tools, runtime and cover.png together.
GameData is created locally; keep it with the launchers when moving the game.
The original ISO is not needed after a successful import. No original disc
payloads, Xbox executable, ISO, videos, saves or diagnostic logs are included.
The original textures, fonts, levels and audio still require your imported ISO.
Spanish translation data is included in the main download. The first Play
after import prepares it from your game automatically; no separate Spanish
download is needed.
Only translation data is bundled, not complete translated game archives.
Single-player and Co-op use this same launcher and main menu. No separate co-op
executable or private launch flag is needed. First Play may take longer while
preparing character and animation resources locally from your imported game.
Approved restored HD UI PNGs are bundled in runtime/hd-ui. They replace only
matching original UI textures when HD UI is enabled; other UI art is prepared
locally from your game. Missing or invalid restored images keep the source art.
Keep the entire runtime folder, including hd-ui, with the launcher. The launcher
verifies the bundled files and asks you to re-extract the ZIP if it is damaged.
The bundled shader seed is installed automatically on Play, including when
updating an existing GameData folder. It prepares known shaders before gameplay
and keeps your learned shader cache. It contains no GPU-specific driver cache.
On a different Windows shader compiler, first startup may take longer while
the seed is compiled locally. Previously unseen shaders and other first-use
work can still cause pauses; the seed does not cover every scene in the game.

SETTINGS AND SAVES
Controls has separate Next Firearm and Previous Firearm keyboard/mouse bindings.
Next keeps your existing Firearms assignment. Previous defaults to Wheel Down
unless that input is already assigned in your saved profile; both are editable.
Character collision with ragdoll bodies is restored. Ragdoll amount controls
body retention independently: Default, More or Forever.
Settings: GameData\xbe-runtime\settings.toml
Saves: GameData\xbe-runtime\hdd
Imported assets: GameData\installed\game_files
Shader cache: GameData\xbe-runtime\shader-cache
To update, extract the new release into a new folder and COPY your GameData
folder beside the new launchers. Keep a backup before deleting an old copy.
On the first ISO import, matching saves, settings and cache from the older
%LOCALAPPDATA%\UrbanChaosBurnerEdition installation are copied automatically.
The old installation is left untouched. Existing portable saves take priority.
Play always uses this folder's GameData, never the older AppData installation.
Saves from the earlier pre-XBE port stay separate and are not migrated.
The launcher has an Open Saves button.
Choose the original game language in Options > Language, then restart the game.
The supported disc supplies English, French, Italian and German. Spanish is not
present in this disc's text/voice banks, regardless of some ISO filenames.
The bundled community translation adds Spanish game text, PC settings and
control descriptions. Select Spanish in Setup or Options > Language (see below).

LOCAL CO-OP AND HEALTH PACKS
Connect one controller, start normally, then choose Co-op on the main menu.
Player 1 uses keyboard/mouse and the upper view; Player 2 uses the controller and
the lower view. Select a character for each player, mark both Ready, then choose
a campaign level. This is local split-screen, not online multiplayer.
Opening either pause menu pauses both players. Controls and world/weapon FOV
edits apply to the player who paused; other graphics settings are shared.
Shared objectives appear in both views. Personal combat and interaction prompts
follow their player. Authored in-game cutscenes use one fullscreen view.

Each fresh level starts each player with one stored health pack, up to three.
Get more from a medic while their original supplies last. Assign Use health pack
in Controls (keyboard default H; set a controller binding for Player 2).
Use a pack to heal yourself or revive a nearby downed partner. A downed player
has ten seconds to be revived; expiry or both players falling fails the mission.
The downed view pulses red and blurs; shared audio stays clear for the partner.
Stored packs also work in single-player. Existing checkpoints without stored-pack
data restore zero packs; they do not grant another free pack on every reload.

MOVIES
Graphics > FMV scaling has Fill and 4:3 modes. Fill keeps picture proportions
and crops the top and bottom to fill the widescreen view. 4:3 keeps the complete
picture with side bars. Bars already present in the source movie may remain.

NOTES
Switch 2 USB support is experimental: the native SDL USB driver and libusb DLL
are included, with Nintendo button prompts. Bluetooth is not implemented.
A data-capable USB cable and a Windows-compatible USB device driver are needed;
this package does not install or replace system drivers. Hardware testing is
still pending. Mouse users get clickable level and rewards browsers.

Requires Windows 10/11 x64 and a Direct3D12-capable GPU. The required Microsoft
Visual C++ runtime DLLs are included beside the game; no system installer runs.
Dynamic scenery lighting, character lighting, effects and retained ragdolls can
affect performance. Graphics options include lighting controls, MSAA, internal
resolution and ragdoll retention.
Normal gameplay writes no diagnostic logs or captures. A failed run can leave a
bounded last-error.txt beside the imported data to explain the failure.

OPTIONAL HD GAME TEXTURES
Download the separate Burner Edition HD texture pack ZIP. In Burner Edition
Setup, choose Install HD Texture Pack and select that ZIP (do not extract it).
Then start the game and enable Graphics > HD game textures > Apply.
The option is grayed out when the pack is not installed or is incomplete.
The pack stays in portable GameData and is separate from HD HUD and menus.
HD textures use more video memory. Original textures remain available with Off.
The unchanged 0.06 HD game texture pack is compatible with 0.08 on both platforms.
Existing installations do not need to reinstall it for this update.
HD game texture artwork: xXthe.RockoXx; converted from the PS2 HD texture pack.

OPTIONAL SPANISH VOICES
Spanish text and PC-menu translations remain included in the main build.
Import your Xbox ISO first and close the game. In Burner Edition Setup choose
IMPORT SPANISH VOICES (PS2 ISO) and select your own European multilingual PS2
Urban Chaos ISO (SLES-53991). If it is in a ZIP or RAR, extract the ISO first.
Setup verifies the disc and converts its Spanish dialogue and cutscene audio
locally. Allow at least 8.4 GiB of free space for conversion. This can take a
while; keep Setup open until it finishes. You can cancel
without replacing your previous voice installation. Then choose Spanish and Play.
No PS2 audio files or downloadable voice pack are included. The conversion tools
are bundled; FFmpeg does not need to be installed separately. Original game
files, other languages, saves and settings are kept. Previously installed voices
remain usable. Without imported voices, Spanish text uses English audio.
Windows and Proton use the same import process (see README-linux.txt).
The dub is from the European PS2 edition; ZARF's text translation is separate.
Two cutscene voice tracks are slightly accelerated without changing pitch to
fit the Xbox video timing. Clips that cannot be matched safely retain their
original recording. In-game audio and lip synchronization need testing.

KNOWN ISSUES
HD HUD still has prompt/display issues; the original HUD remains available.
Co-op is experimental, with incomplete campaign/character/turret gameplay coverage.
A reported co-op aiming offset remains under investigation.
The intermittent missing injured officer in Angel Heights remains under investigation.
The targeted stone-ledge mapping repair still needs confirmation at the reporter's exact wall.
Spanish text and imported voice synchronization need community review.
A Linux tester confirmed setup and gameplay in the earlier compatibility build;
that does not verify this release's new features or co-op under Proton.

CREDITS
XboxXBERecomp: TGP482. Earlier xboxrecomp work and retained helpers: sp00nz.
Additional dependency notices are in licenses/. This is an unofficial fan port.
Original game content belongs to its respective owners and must be supplied
locally by the user.

INTEGRATED COMMUNITY SPANISH TEXT
The ZARF beta translation and supplemental translations are included as text
data. On the first Play after import or upgrade, the launcher builds the
translated files locally from your imported game in
GameData/xbe-runtime/translations/es-zarf. English voices are retained unless
Spanish voices have been imported from your European PS2 ISO.
Choose Espanol (ZARF beta) in Options > Language, Apply, then restart. You can
also select Spanish in Burner Edition Setup before pressing PLAY.
The translation fills remaining English messages and includes Spanish PC menus
and control descriptions. Additional translations still need community review.

