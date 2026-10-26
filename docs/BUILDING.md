# Source and build status

The 0.08 release uses XboxXBERecomp by TGP482 for static recompilation and its
Direct3D 12 host runtime. Exact dependency identities and selected backports are
in `dependencies.lock.json`. The active entry point is
`scripts/build_xbe_game_trial.py`. Edit maintained sources, not generated copies.

## Source snapshot

This snapshot is based on the refreshed published 0.07 source archive, SHA256
`a651ab9f89009430600f2c730b5f84f394ee20c7803e9c642d4660ea54a79c70`, from
commit `2a689f0b0878d6217df9b8a5b5783898d0dc04d2`. Current build and setup source
closure, tests, launcher code, shared manifests and current documentation overlay
that baseline. The companion source audit records every file hash and all changes.

The active native dependency subset includes archive/texture parsing, model and
animation loading, pose evaluation and preview geometry used by co-op character
selection. These files are not all historical. The root CMake project, unrelated
native reconstruction and older `src/`/`shaders/` experiments are retained for
reference; they do not replace the current build driver.

Original Xbox/PS2 disc assets, executables, original images/audio/movies/fonts,
generated title C++, private captures, saves and compiled programs are excluded.
Runtime co-op character/turret/injection archives and optional Spanish voices are
derived locally from the user's discs. The snapshot includes the converters and
identity recipes, not their game-data output. Approved restored HD UI PNGs and the
custom Burner Edition cover/icon are the artwork exceptions.

The portable shader seed is an explicitly validated binary exception: exact
compiler bytecode, source identity and hashes under `packaging/shader-seed/`.
It replaces the previous seed as a unit. No GPU-specific pipeline blobs or
player-learned caches are included. `scripts/build_xbe_shader_seed.py` and its
test cover the seed producer.

## Full runtime build limitation

A clean checkout does **not yet reproduce the complete game executable**.
This is an inspection/contribution snapshot, not a self-contained reproducible
binary build. The current driver requires an already qualified private workspace
at `build/xbe-full-integration-20261014`, including scanned title configuration,
generated title units, original generation backups, adapters and verification
inputs. It also contains environment-specific compiler, SDK, Python and zlib
paths. Bootstrapping the qualified title inputs from a new user's disc remains
unfinished. Do not fabricate missing qualification files or call the historical
root CMake target a reproduction of this release.

The SDK is an unmodified checkout of
`c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7` at
`build/upstream-review-xboxxbe-20261014`. The maintained `prepare_xbe_*.py`
scripts stage checked transformations in the build driver's explicit order.
With qualified inputs present, close the game and use the serialized wrapper
`scripts/run_animation_resume.ps1` with `UCRR_XBE_RELEASE=1`, then clear that
environment variable. The driver builds the release runtime and executes its
registered fixtures. Routine tracing and capture diagnostics are compiled out;
bounded fatal-error reporting and the normal FPS display remain.

Selected legacy input, prompt-art and shader-cache helpers are retained under
`dependency-overrides/xboxrecomp/`. They do not replace the current SDK. Run
`scripts/setup_xbe_libusb.py` when the pinned libusb dependency is absent; it
verifies libusb 1.0.29, requires 7-Zip, and does not install USB system drivers.
Switch 2 hardware validation is pending and Bluetooth is unsupported.

## Setup, packaging and languages

On Windows, `launcher/burner-xbe/build.ps1 -Package <package-directory>` builds
Setup and direct Play with the .NET Framework C# compiler. It uses the edition
icon in `launcher/burner/` and does not require original game assets. The old
launcher folder also retains the supported disc manifest and historical code.

`launcher/burner-xbe/package.py` requires an explicit runtime and expected SHA256,
refuses to overwrite an existing package, and copies an exact file allowlist.
Build both launchers, then run its `--audit` and
`test_setup.py --package <package-directory>` checks. Local Python, zlib,
Microsoft redistributable and optional voice-conversion tool paths need adapting
on another machine. Never archive the entire development workspace as a release.

Normal Play supports both Single player and Co-op from the main menu. The worker
sanitizes diagnostic environment variables rather than inheriting development
launch settings. Opening Co-op does not activate gameplay until readiness and
resource validation succeed. Do not add a separate private co-op startup path.

Spanish text is bundled as translated strings and combined with locally imported
Xbox archives. Optional voices are extracted and converted from the user's
supported European PS2 ISO using `spanish_voice_iso.py`, `ps2_audio.py`, the other
voice helpers and the pinned tools in `packaging/spanish-voice-tools.json`.
The release bundles FFmpeg executables/libraries with their license and build
notices; this source archive includes only the manifest, conversion source and
notices. No original or converted game audio is distributed. Legacy delta-pack
utilities retained in source are not the supported public installation route.

## Windows and Linux/Proton verification

Maintain both packages with the same version, maintained source, runtime and
asset manifests. Linux runs that Windows runtime through Steam/Proton; it is not
native. `launcher/burner-linux/` provides instructions and tested command/path
handling. The user supplies Steam/Proton and Vulkan drivers. The recommended
Steam entry uses the bundled Python worker, avoiding a dependency on the Windows
.NET setup window.

The matching 0.08 production runtime build and registered offline checks passed
in the qualified Windows workspace, including hardware/WARP rendering fixtures,
co-op/menu ownership checks and quiet-release diagnostics. The artifact identity
and bounded acceptance summary are in [VERIFICATION-0.08.json](VERIFICATION-0.08.json).
That does not establish a clean source-only rebuild or live gameplay acceptance.
A bounded Windows visual smoke test of the exact packaged runtime passed on
2026-10-26: visible boot, mouse menus, Single Player Hope Street and Canals,
Co-op Hope Street with separate views, Player 1 firing/pause/FOV, and return from
Co-op to Single Player. Physical Player 2 input/pause ownership, audio quality,
frame pacing, death/revive and full-campaign paths were not exercised in that pass.
Live Proton remains untested for 0.08. An earlier Linux compatibility build had
tester-confirmed gameplay; new co-op and language features need separate Proton
acceptance. See VERIFICATION-0.08.json for the recorded evidence and limits.

Tests using original instructions/assets require the supported local discs.
Original-body preview checks have a standalone runner in
`scripts/xbe_coop_preview_assets_checks/`; its CMake target requires MSVC and a
local zlib DLL/header pair. No original resource is copied into this snapshot.
Do not substitute an offline geometry/input test for visual, audio, control,
campaign or language acceptance. The documented co-op aiming report remains open.
