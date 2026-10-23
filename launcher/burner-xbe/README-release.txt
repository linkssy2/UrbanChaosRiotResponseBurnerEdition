Urban Chaos: Riot Response - Burner Edition 0.07

HD HUD is still a work in progress and has known prompt/display issues.
Turn HD UI Off to use the original HUD, which works normally.

PC PORT
Windows x64 PC port using XboxXBERecomp by TGP482, with native Direct3D12 rendering,
PC controls and menus. Original gameplay is statically recompiled from Xbox code.
XboxXBERecomp: https://github.com/TGP482/XboxXBERecomp

NEW IN 0.07 (SINCE 0.06)
- Fixed widescreen rifle-scope edge coverage and incomplete red tint.
- Ladder views follow your FOV; improved climbing/shield-bash arm coverage and sleeve mapping, and corrected taser-arc alignment at changed FOV.
- Fixed invisible restart confirmations and Difficulty-menu flicker. Mission/checkpoint restart now use mouse-friendly dialogs; controller layouts remain available.
- Fixed the reported dynamic wall-lighting flicker and restored original depth-clamp handling.
- Added a targeted stone-ledge texture-mapping repair for original and HD textures.
- Fixed disappearing blue pipe flames caused by exhausted renderer registrations; no texture-pack reinstall needed.
- Added automatic shader warm-up for known shaders to reduce first-use stutters while preserving your learned cache.
- Reduced repeated audio decoding using a selected upstream improvement and reduced unnecessary keyboard polling.
- Added separate Next Firearm/Previous Firearm bindings and restored character collision with ragdoll bodies.
- Integrated Spanish game text, PC menus/control descriptions and launcher language selection. Translation data is bundled and prepared automatically from your imported ISO; English voices remain.
- Separate experimental Linux package runs this Windows build through Proton; it is not a native Linux port and has not been gameplay-tested on Linux.

INSTALL
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
The 0.06 HD game texture pack remains compatible; its artwork has not changed.
HD game texture artwork: xXthe.RockoXx; converted from the PS2 HD texture pack.

KNOWN ISSUES
HD HUD still has prompt/display issues; the original HUD remains available.
The intermittent missing injured officer in Angel Heights remains under investigation.
The targeted stone-ledge mapping repair still needs confirmation at the reporter's exact wall.
Spanish additions need community review. The experimental Linux package is unverified in gameplay.

CREDITS
XboxXBERecomp: TGP482. Earlier xboxrecomp work and retained helpers: sp00nz.
Additional dependency notices are in licenses/. This is an unofficial fan port.
Original game content belongs to its respective owners and must be supplied
locally by the user.

INTEGRATED COMMUNITY SPANISH TEXT
The ZARF beta translation and supplemental translations are included as text
data. On the first Play after import or upgrade, the launcher builds the
translated files locally from your imported game in
GameData/xbe-runtime/translations/es-zarf. English voices are retained.
Choose Espanol (ZARF beta) in Options > Language, Apply, then restart. You can
also select Spanish in Burner Edition Setup before pressing PLAY.
The translation fills remaining English messages and includes Spanish PC menus
and control descriptions. Additional translations still need community review.

0.07 PACKAGING CORRECTION
The refreshed 0.07 downloads include Spanish translation data that was omitted
from the first 0.07 packages. To update, replace the release files while keeping
GameData, or copy GameData into a fresh extraction as described above. The next
Play, either in Setup or through the Play launcher, prepares it automatically.
Your saves, other settings and installed HD game textures are preserved.
