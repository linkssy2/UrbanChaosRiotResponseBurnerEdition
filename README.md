# Urban Chaos: Riot Response — Burner Edition

**Version 0.01 · Windows x64**

An unofficial PC port built with xboxrecomp and native PC engine components.
Original gameplay and mission logic use statically recompiled Xbox code, with
PC rendering, input and menu improvements. This is a hybrid recompilation;
it is not a complete ground-up rewrite.

## Install

1. Download the **0.01 Win64 ZIP** from Releases and extract the entire folder.
2. Run **Burner Edition Setup.exe** and select your supported original Xbox ISO.
3. After import, run **Play Burner Edition.exe** for direct launch.

No original game assets, ISO, Xbox executable, videos or saves are supplied.
Your disc is read locally. Imports, settings and saves live in
`%LOCALAPPDATA%\UrbanChaosBurnerEdition`. Existing installations are reused.

## Performance

**Dynamic scenery lighting and character lighting can cause slowdowns**, especially
with multiple characters and effects. Lower Character lighting quality or disable
these lighting options in Graphics, then choose **Apply**. Stable 60 FPS is not guaranteed.
Routine runtime logs and diagnostic recorders are disabled in the release.

Mouse/keyboard and controller input are supported. Graphics and control changes
use Apply. Version 0.01 is an early release; please report the mission, settings
and steps when reporting bugs. Do not attach game files or saves containing personal data.

## Development

See [the code map](docs/CODE_MAP.md), [source/build limits](docs/BUILDING.md) and
[credits](CREDITS.md). Generated game code and local research/build artifacts
are intentionally excluded. The source snapshot is for inspection and contribution;
a standalone reconstruction of the complete release build is not yet provided.
