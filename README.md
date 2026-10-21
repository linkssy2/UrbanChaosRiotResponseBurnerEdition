# Urban Chaos: Riot Response â€” Burner Edition

**PC port Â· 0.06 Â· Windows x64 release**

An unofficial PC port powered by [XboxXBERecomp by TGP482](https://github.com/TGP482/XboxXBERecomp), with native Direct3D 12 rendering and statically recompiled original Xbox gameplay. Earlier Burner Edition work and retained helpers use [xboxrecomp by sp00nz](https://github.com/sp00nznet/xboxrecomp).

## New in 0.06

- More mouse-friendly menus, scrollable Controls, separate Reload/Interact bindings and a lower mouse sensitivity slider.
- Improved FMV pacing and hold-to-skip confirmation, widescreen scope masking, higher FPS limits and configurable pistol spin.
- Fixed blue pipe flames with HD HUD enabled. Optional HD game textures by **xXthe.RockoXx** are a separate download installed through Setup.
- Optional Spanish beta translation support by **ZARF**; English voices, translation files separate.

**HD HUD still has known prompt/display issues. The original HUD works normally with HD UI Off.**

See [all fixes and additions since 0.05](docs/CHANGES-0.06.md).

## Install

1. Extract the entire matching **0.06 Windows x64 ZIP**.
2. Run **Burner Edition Setup.exe** and select your supported original Xbox ISO or XISO.
3. After import, use **Play Burner Edition.exe** to launch directly.

Requires Windows 10/11 x64 and a Direct3D 12-capable GPU. No original game executable, ISO, extracted UI art, videos or saves are included. Original textures, fonts, levels and audio still come from your disc. This release includes newly restored HD replacement artwork and the existing custom Burner Edition cover/icon; see `artwork/README.md`.

Settings and saves live in `GameData\xbe-runtime` (profiles inside `hdd`). For updates, copy your existing `GameData` beside the new launchers. On the first ISO import, compatible XBE saves, settings and cache from `%LOCALAPPDATA%\UrbanChaosBurnerEdition` are copied without changing the old installation. Existing portable data takes priority. Play never falls back to AppData. Pre-XBE saves stay separate. Setup includes **Open Saves**. Extract to a writable folder; the original ISO is unnecessary after import.

Choose the supplied original language in **Options â†’ Language**, then restart: English, French, Italian or German. Spanish is available through the optional community beta by ZARF, with English voices; translation files are separate. Custom PC setting descriptions remain in English.

Dynamic scenery lighting, character lighting, effects and retained ragdolls can still cause slowdowns. Routine release diagnostics are disabled; failed launches can leave a bounded error report. Switch 2 USB support remains experimental and untested on hardware; Bluetooth is unsupported.

## Optional HD game textures

Download the separate **0.06 HD Game Textures ZIP**. In **Burner Edition Setup**, choose **Install HD Texture Pack** and select the ZIP without extracting it. Restart the game, enable **HD game textures** in Graphics, and Apply. The option is unavailable until installed. This pack replaces game textures only; HD HUD is a separate setting. Credit: **xXthe.RockoXx**, original PS2 U.C.riot.v.1.2.HD.textures pack, converted for this port.

## Development

See [the code map](docs/CODE_MAP.md), [runtime maintenance](docs/XBE-RUNTIME-MAINTENANCE.md), [source/build status](docs/BUILDING.md) and [credits](CREDITS.md). Maintained source and tests are supplied for inspection and contribution. A clean checkout cannot yet rebuild the complete game without the private generated title inputs described in the build notes.
