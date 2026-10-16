Urban Chaos: Riot Response - Burner Edition 0.03

PC PORT
Windows x64 PC port using XboxXBERecomp by TGP482, with native Direct3D12 rendering,
PC controls and menus. Original gameplay is statically recompiled from Xbox code.
XboxXBERecomp: https://github.com/TGP482/XboxXBERecomp

INSTALL
1. Extract the entire ZIP to a folder.
2. Run Burner Edition Setup.exe and choose your supported original Xbox ISO.
   Both the verified full-disc ISO and extracted .xiso.iso layouts are accepted.
3. Once imported, use Play Burner Edition.exe to play without opening setup.
   Keep the EXEs, tools, runtime and cover.png together.
Existing verified Burner Edition game imports are reused. No game assets, Xbox
executable, ISO, videos, saves or diagnostic logs are included in this package.
Menu fonts, HUD artwork, textures and audio are read from your imported game.

SETTINGS AND SAVES
The new runtime stores settings and saves in:
%LOCALAPPDATA%\UrbanChaosBurnerEdition\xbe-runtime
Its saves are inside hdd. Old Burner Edition saves/settings are kept separate
and are not automatically migrated. The launcher has an Open Saves button.
Languages available on the supported disc: English, French, Italian and German.
Language changes require restarting the game; Spanish is not present in this
disc's text/voice banks, regardless of some ISO filenames.
Custom PC settings and controls descriptions remain in English.

NOTES
Requires Windows 10/11 x64 and a Direct3D12-capable GPU. The required Microsoft
Visual C++ runtime DLLs are included beside the game; no system installer runs.
Dynamic scenery lighting, character lighting, effects and retained ragdolls can
affect performance; stable 60 FPS is not guaranteed. Graphics options
include lighting controls, MSAA, internal resolution and ragdoll retention.
Normal gameplay writes no diagnostic logs or captures. A failed run can leave a
bounded last-error.txt beside the imported data to explain the failure.

CREDITS
XboxXBERecomp: TGP482. Earlier xboxrecomp work and retained helpers: sp00nz.
Additional dependency notices are in licenses/. This is an unofficial fan port.
Original game content belongs to its respective owners and must be supplied
locally by the user.
