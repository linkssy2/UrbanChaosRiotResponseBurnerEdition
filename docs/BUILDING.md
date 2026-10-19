# Source and build status

The 0.05 release uses XboxXBERecomp by TGP482 for static recompilation and
the Direct3D 12 host runtime. Exact dependency versions are in
`dependencies.lock.json`. The current entry point is
`scripts/build_xbe_game_trial.py`; the root CMake project and `src/`, `native/`
and `shaders/` retain earlier development work.

## Included source

The snapshot includes the current build driver's maintained source closure,
checked runtime transformations, regression fixtures, launcher/import code and
dependency notices. It includes the HD UI service and rendering integration,
crosshair/FOV controls, frontend fonts, and accepted vertex, texture, sampler
and visibility-query changes. Edit maintained files under `scripts/`, not
generated copies under `build/`.

Original disc assets, Xbox executables, generated title C++, extracted textures,
fonts, disassembly, private captures, player saves and compiled test programs
are excluded. The custom Burner Edition cover and icon are retained. Tests that
need original instructions or resources read the user's local supported game.

## Launcher and package

On Windows, `launcher/burner-xbe/build.ps1 -Package <package-directory>` builds
Setup and direct Play with the .NET Framework C# compiler. It uses the edition
icon in `launcher/burner/` and does not require original game assets. The latter
folder also retains the historical launcher, disc manifest and custom artwork.

`launcher/burner-xbe/package.py` requires an explicit runtime and expected SHA256,
refuses to overwrite an existing package, and copies an exact dependency
allowlist. Build both launchers, then run the packager's `--audit` mode and
`test_setup.py --package <package-directory>`. Packaging selects verified local
Python, zlib and Microsoft redistributable files; their paths need adjustment on
another machine. Never archive the development workspace as a release.

## Full game build limitation

A clean checkout does **not yet reproduce the complete game executable**. The
driver requires an already qualified private workspace at
`build/xbe-full-integration-20261014`, including scanned title configuration,
generated title units, original generation backups, title adapters and local
verification inputs. Compiler, Windows SDK, Python and zlib paths are also local.
Bootstrapping these inputs from a user's disc without that existing workspace
remains unfinished. Do not fabricate a qualification report or treat the root
historical build as a reproduction of this candidate.

The XboxXBERecomp SDK is an unmodified checkout of
`c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7` at
`build/upstream-review-xboxxbe-20261014`. Maintained `prepare_xbe_*.py` scripts
apply checked overlays to private outputs in the build driver's explicit order.
With the qualified inputs present, close the game and use the existing serialized
heavy-build wrapper with `UCRR_XBE_RELEASE=1`; clear that environment variable
afterwards. The driver builds the runtime and runs its registered fixtures.
Routine tracing, capture hotkeys and diagnostic timing are compiled out in a
release; bounded fatal-error reporting and the normal FPS display remain.

Selected historical keyboard/controller, prompt-art and shader-cache helpers
are preserved in `dependency-overrides/xboxrecomp/` against the older pinned
runtime. They do not replace the current SDK. Keep all dependency notices.

Run `scripts/setup_xbe_libusb.py` before building when the pinned libusb dependency
is absent. It fetches and verifies libusb 1.0.29; it requires 7-Zip and does not
install Windows USB drivers. Switch 2 USB hardware validation is pending and
Bluetooth is unsupported.

## Verification scope

The snapshot's companion audit records every source hash, the previous source
archive identity, current closure and disc-payload exclusion check. Build and
package results belong to the matching candidate's verification records; this
document does not substitute an older release's test count or executable hash.
Offline fixtures, full private-workspace compilation and live visual/audio/input
acceptance are separate. This is not a clean Windows VM qualification, an
independent source-only rebuild, or an exhaustive campaign/language playthrough.
HD UI has known prompt/display issues; the original HUD remains available with
HD UI Off. Offline coverage does not establish complete live visual acceptance.
