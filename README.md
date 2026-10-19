# Urban Chaos: Riot Response — Burner Edition

**PC port · 0.05 · Windows x64 release**

An unofficial PC port powered by [XboxXBERecomp by TGP482](https://github.com/TGP482/XboxXBERecomp), with native Direct3D 12 rendering and statically recompiled original Xbox gameplay. Earlier Burner Edition work and retained helpers use [xboxrecomp by sp00nz](https://github.com/sp00nznet/xboxrecomp).

## New in 0.05

- Corrected widescreen interaction-marker placement, with the configured mouse/key icon for Display Orders.
- Portable installation: imports, profiles, settings and shader cache live in `GameData` beside the launchers. Move the whole folder together.
- Optional restored HD weapon/ammo icons, medals and menu photographs, with sharper original fonts and action symbols. HD UI defaults to Off.
- Mouse-friendly main, pause and settings menus in the game's style. Controllers use the original game menus; added Graphics and Controls pages remain available.
- Mouse-friendly pause Quit confirmation retains original sounds and callbacks; controllers keep the original dialog.
- Separate weapon FOV from 50-150% (100% keeps each authored weapon view), without changing the world camera.
- Vertical FOV from 40–90 degrees (default 53), widescreen HUD placement, and crosshair size from 50–200% (original size 100%).
- Shared renderer improvements reduce repeated vertex, texture and visibility-query work across levels. Results depend on scene and hardware; stable 60 FPS is not guaranteed.

**HD HUD is still a work in progress and has known prompt/display issues. Turn HD UI Off to use the original HUD, which works normally.**

See [all fixes and additions since 0.04](docs/CHANGES-0.05.md).

## Install

1. Extract the entire matching **0.05 Windows x64 ZIP**.
2. Run **Burner Edition Setup.exe** and select your supported original Xbox ISO or XISO.
3. After import, use **Play Burner Edition.exe** to launch directly.

Requires Windows 10/11 x64 and a Direct3D 12-capable GPU. No original game executable, ISO, extracted UI art, videos or saves are included. Original textures, fonts, levels and audio still come from your disc. This release includes newly restored HD replacement artwork and the existing custom Burner Edition cover/icon; see `artwork/README.md`.

Settings and saves live in `GameData\xbe-runtime` (profiles inside `hdd`). For updates, copy your existing `GameData` beside the new launchers. On the first ISO import, compatible XBE saves, settings and cache from `%LOCALAPPDATA%\UrbanChaosBurnerEdition` are copied without changing the old installation. Existing portable data takes priority. Play never falls back to AppData. Pre-XBE saves stay separate. Setup includes **Open Saves**. Extract to a writable folder; the original ISO is unnecessary after import.

Choose the supplied original language in **Options → Language**, then restart: English, French, Italian or German. Spanish is not present in the supported disc's text/voice banks. Custom PC setting descriptions remain in English.

Dynamic scenery lighting, character lighting, effects and retained ragdolls can still cause slowdowns. Routine release diagnostics are disabled; failed launches can leave a bounded error report. Switch 2 USB support remains experimental and untested on hardware; Bluetooth is unsupported.

## Development

See [the code map](docs/CODE_MAP.md), [runtime maintenance](docs/XBE-RUNTIME-MAINTENANCE.md), [source/build status](docs/BUILDING.md) and [credits](CREDITS.md). Maintained source and tests are supplied for inspection and contribution. A clean checkout cannot yet rebuild the complete game without the private generated title inputs described in the build notes.
