# Urban Chaos: Riot Response — Burner Edition

**PC port · Version 0.03 · Windows x64**

An unofficial PC port powered by [XboxXBERecomp by TGP482](https://github.com/TGP482/XboxXBERecomp), with native Direct3D 12 rendering and statically recompiled original Xbox gameplay. Earlier Burner Edition work and retained helpers use [xboxrecomp by sp00nz](https://github.com/sp00nznet/xboxrecomp).

## What's new

- New rendering/runtime foundation, with improved performance in testing.
- Game-style mouse menus, keyboard/controller rebinding, profile management and PC button prompts.
- Widescreen, resolution scaling, MSAA, filtering, lighting, fog and ragdoll options.
- Dialogue/prompt, audio, fog and HUD fixes; English, French, Italian and German language selection in **Options → Language** (restart required).
- Updated ISO setup and direct Play launcher, bundled C++ runtime DLLs, and routine diagnostics disabled.

## Install

1. Download the **0.03 Windows x64 ZIP** from [Releases](https://github.com/linkssy2/UrbanChaosRiotResponseBurnerEdition/releases/latest) and extract the entire folder.
2. Run **Burner Edition Setup.exe** and select your supported original Xbox ISO or XISO.
3. After import, use **Play Burner Edition.exe** to launch directly.

Requires Windows 10/11 x64 and a Direct3D 12-capable GPU. No original game assets, Xbox executable, ISO, videos or saves are supplied. Menu fonts, HUD artwork, textures and audio come from your imported disc. Keep the extracted package together.

Existing verified game imports are reused. This runtime keeps its settings and saves in `%LOCALAPPDATA%\UrbanChaosBurnerEdition\xbe-runtime`; older releases' saves/settings are preserved separately and are **not automatically migrated**. Setup includes **Open Saves**.

## Notes

Dynamic scenery lighting and character lighting can still cause slowdowns, especially during busy scenes and explosions. Effects and retained ragdolls also affect performance; stable 60 FPS is not guaranteed. Adjust Graphics settings and choose **Apply**.

Custom PC settings/control descriptions remain in English. Spanish is not present in the supported disc's text/voice banks. Routine logs and diagnostic captures are disabled; a failed launch can leave a small error report.

## Development

See [the code map](docs/CODE_MAP.md), [runtime maintenance](docs/XBE-RUNTIME-MAINTENANCE.md), [source/build notes](docs/BUILDING.md) and [credits](CREDITS.md). Generated game code, disc assets and private research/build outputs are excluded. The maintained source is provided for inspection and contribution; the standalone rebuild pipeline still needs work.
