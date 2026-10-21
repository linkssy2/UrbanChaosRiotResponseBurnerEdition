Urban Chaos: Riot Response - Burner Edition 0.06

HD HUD is still a work in progress and has known prompt/display issues.
Turn HD UI Off to use the original HUD, which works normally.

PC PORT
Windows x64 PC port using XboxXBERecomp by TGP482, with native Direct3D12 rendering,
PC controls and menus. Original gameplay is statically recompiled from Xbox code.
XboxXBERecomp: https://github.com/TGP482/XboxXBERecomp

NEW IN 0.06
- Mouse-friendly Profile Select, Difficulty, Mission Failed, Objectives and quit menus; original controller layouts retained.
- Removed underlying controller-menu rows from mouse menus and their transitions.
- Organized, scrollable Controls with separate Reload/Interact bindings and merged game/controller options.
- Mouse sensitivity slider supports settings down to 1%.
- Hold Enter or left-click to skip FMVs, with a bottom-right progress bar; release between videos.
- Improved FMV pacing, menu input transitions and title-screen any-key/click support.
- Fixed widescreen rifle scope masking and the gameplay cap above 60 FPS.
- Pistol spin supports the original gesture or an assignable button.
- Fixed missing blue pipe flames when HD HUD is enabled.
- Optional HD game textures: separate ZIP, installed through Setup. Option stays unavailable until installed.
- Optional Spanish beta support by ZARF (English voices; translation files separate).
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
Approved restored HD UI PNGs are bundled in runtime/hd-ui. They replace only
matching original UI textures when HD UI is enabled; other UI art is prepared
locally from your game. Missing or invalid restored images keep the source art.
Keep the entire runtime folder, including hd-ui, with the launcher. The launcher
verifies the bundled files and asks you to re-extract the ZIP if it is damaged.

SETTINGS AND SAVES
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
Custom PC settings and controls descriptions remain in English.

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
HD game texture artwork: xXthe.RockoXx; converted from the PS2 HD texture pack.

CREDITS
XboxXBERecomp: TGP482. Earlier xboxrecomp work and retained helpers: sp00nz.
Additional dependency notices are in licenses/. This is an unofficial fan port.
Original game content belongs to its respective owners and must be supplied
locally by the user.

Optional community Spanish text
The ZARF beta translation is supported as a separately converted local pack in
GameData/xbe-runtime/translations/es-zarf. Choose Espanol (ZARF beta) in
Options > Language, Apply, then restart. English voices and untranslated text
are retained. PC-specific menus remain English. No translated game archives
are included in the base package.
