# Source and build status

Version 0.03 uses [XboxXBERecomp](https://github.com/TGP482/XboxXBERecomp)
by TGP482 for static recompilation and the Direct3D12 host runtime. The exact SDK
revision and third-party versions are recorded in `dependencies.lock.json`.
`src/`, `native/` and the root CMake project also retain earlier development work;
the current executable is built through `scripts/build_xbe_game_trial.py`.

## What is included

The repository contains maintained host integration, preparation scripts,
regression fixtures, launcher/import code and dependency notices. Preparation
scripts apply checked edits to private generated output; they are the source to
edit, rather than a generated file under `build/`. Selected input, prompt drawing
and shader-cache helpers are retained from the earlier sp00nznet/xboxrecomp work
under `dependency-overrides/xboxrecomp/`.

Original disc assets, original executable images, generated title C++, extracted
fonts/textures, disassembly, machine-specific captures and player saves are not
part of this snapshot. Tests that need original instructions or resources read
them from a locally supplied supported disc; they do not embed those test inputs.

## Rebuilding the launcher

On Windows, `launcher/burner-xbe/build.ps1 -Package <package-directory>` builds
both Setup and direct Play with the .NET Framework C# compiler. It needs the
edition icon in `launcher/burner/` but no game assets. Keep the supporting folders
from the release ZIP beside those EXEs. `launcher/burner/` also contains the older
launcher and shared disc manifest/artwork; it is not the current entry point.

`launcher/burner-xbe/package.py` requires an explicitly selected runtime and its
expected SHA256, and refuses to overwrite an existing package. It copies only
its allowlisted dependencies. Compile both launchers, then run its `--audit`
mode and `test_setup.py --package <package-directory>` before publishing. The
packager currently has local paths for embedded Python, zlib and Microsoft's
redistributable files; point those at verified local copies when using another
machine. Never distribute the development workspace as a release archive.

## Full game build limitation

A clean checkout does **not yet reproduce the complete game executable**. The
current build driver expects an already qualified private workspace at
`build/xbe-full-integration-20261014`, including the scanned title configuration,
generated title units, original generation backups, title adapters and local
verification inputs. It also selects local compiler, Windows SDK, Python and
zlib locations. Regenerating those inputs from a supplied disc and making that
bootstrap independent of the development workspace remains unfinished.

The current SDK is an unmodified checkout at
`c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7`, staged at
`build/upstream-review-xboxxbe-20261014`. With the qualified private inputs in
place, the existing serialized wrapper runs `scripts/build_xbe_game_trial.py`
with `UCRR_XBE_RELEASE=1`; clear that environment setting afterwards. Do not run
the old root build scripts as a claimed reproduction of 0.03, or fabricate the
qualification report to bypass its checks.

The tested 0.03 game executable SHA256 is
`67878dcdfbaa52a4128ad54165bba994a9b1a24c0058ab3a363ee916ca5f3fad`.
The release preserves that executable unchanged from the accepted candidate.
Routine tracing, capture hotkeys and diagnostic timing are compiled out; bounded
fatal-error reporting and the normal FPS display remain available.

## Verification scope

The candidate passed 55 contract/regression/hardware checks. Setup was checked
with both supported disc layouts, staged import/reuse and dependency audits;
the user tested the candidate and reported smooth gameplay. This is not a clean
Windows VM qualification, a fresh source-only game build, or an exhaustive
campaign and multilingual audio acceptance test.


### 0.04 USB controller dependency
Run `python scripts/setup_xbe_libusb.py` before the current runtime build (requires 7-Zip on PATH). This fetches and verifies pinned libusb 1.0.29 headers, DLL/import library, license and source. The build checks that SDL's libusb backend is enabled. It does not install Windows USB drivers. Switch 2 USB hardware validation remains pending; Bluetooth is unsupported.
