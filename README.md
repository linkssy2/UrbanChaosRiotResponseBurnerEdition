# Urban Chaos: Riot Response - Burner Edition

**PC port - 0.07**

An unofficial PC port powered by [XboxXBERecomp by TGP482](https://github.com/TGP482/XboxXBERecomp), with Direct3D 12 rendering and statically recompiled original Xbox gameplay. Earlier Burner Edition work and retained helpers use [xboxrecomp by sp00nz](https://github.com/sp00nznet/xboxrecomp).

## Changes since 0.06

- Corrected widescreen rifle-scope borders and red tint, ladder and weapon-effect FOV alignment, and extended first-person sleeves with matching fabric mapping.
- Corrected the reported wall-lighting flicker and added mouse-friendly mission/checkpoint restart confirmations.
- Added separate Next Firearm and Previous Firearm bindings and restored character collision with ragdoll bodies.
- Included a portable shader seed to reduce known first-use shader stalls, plus focused audio/input work reductions.
- Expanded Spanish text and PC-menu coverage, with language selection in Setup. The optional ZARF translation pack is still required; voices remain in English.
- Fixed a separate renderer-registration exhaustion case that could hide active blue pipe flames with HD options enabled.

See [the release change list](docs/CHANGES-0.07.md) for the complete scope and remaining reports.

**HD HUD still has known prompt/display issues. Turn HD HUD and menus Off to use the original HUD.**

## Install and update

1. Extract the entire matching Windows x64 ZIP to a writable folder.
2. Run **Burner Edition Setup.exe** and select your supported original Xbox ISO or XISO.
3. After import, use **Play Burner Edition.exe** to launch directly.

Windows requires Windows 10/11 x64 and a Direct3D 12-capable GPU. No original game executable, ISO, extracted UI art, videos or saves are included. Original textures, fonts, levels and audio still come from your disc. Newly restored HD replacement artwork and the existing custom Burner Edition cover/icon are included; see `artwork/README.md`.

Settings and saves live in `GameData/xbe-runtime` (profiles inside `hdd`). To update, extract into a new folder and copy your existing `GameData` beside the new launchers. Keep a backup before removing the older installation. Existing portable data takes priority over first-import migration from the earlier AppData installation. Play does not fall back to AppData. Pre-XBE saves remain separate. Setup includes **Open Saves**. The original ISO is unnecessary after successful import.

The supplied original language banks are English, French, Italian and German. Select a language in Setup or **Options > Language**, then restart. Spanish requires the separately converted community pack by **ZARF**. Supplemental messages, PC menus and control descriptions support Spanish; the additions still need community review.

The bundled shader seed prepares observed shader variants before gameplay and preserves the player's learned cache. It contains no GPU-driver pipeline cache. A different shader compiler rebuilds those records locally; new shader variants and other first-use work can still cause hitches.

## Experimental Linux package

The separate Linux package runs the same Windows executable through user-supplied Steam/Proton. It is not a native Linux port. Its `burner.sh` supports ISO import, optional HD texture installation, language selection and Play without the Windows setup window. See [the Linux instructions](launcher/burner-linux/README-linux.txt). Linux gameplay, rendering, FMVs, audio and controllers remain unverified; the command tests only check dispatch, quoting, path handling and failure propagation.

## Optional HD game textures

The HD game texture pack is a **separate download**. In Setup, choose **Install HD Texture Pack** and select its ZIP without extracting it. Restart the game, enable **HD game textures** in Graphics, and Apply. The option is unavailable until a valid pack is installed. This replaces game textures only; HD HUD is a separate setting. Credit: **xXthe.RockoXx**, original PS2 U.C.riot.v.1.2.HD.textures pack, converted for this port.

## Verification limits

The missing injured officer in Angel Heights remains an unresolved report. The targeted HD wall-ledge repair has offline checks, but the reporter's exact location was unavailable for visual confirmation. Performance depends on settings, GPU and scene; no campaign-wide fixed frame rate is promised. Switch 2 USB support remains experimental and untested on hardware; Bluetooth is unsupported. Routine release diagnostics are disabled; failed launches can leave a bounded error report.

## Development

See [the code map](docs/CODE_MAP.md), [runtime maintenance](docs/XBE-RUNTIME-MAINTENANCE.md), [source/build status](docs/BUILDING.md) and [credits](CREDITS.md). Maintained source and tests are supplied for inspection and contribution. A clean checkout cannot yet rebuild the complete game without the private generated title inputs described in the build notes.
