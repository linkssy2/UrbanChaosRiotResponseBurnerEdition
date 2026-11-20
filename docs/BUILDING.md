# Burner Edition 0.21 build status

This is an inspection/contribution snapshot of the active source closure, not a
self-contained clean-build kit. The current builder still requires qualified
title inputs under `build/xbe-full-integration-20261014`, including private
scanned configuration, generated game units, original-generation backups and
fixture data. It also names workstation compiler/SDK/Python paths. Bootstrapping
those title inputs from a fresh disc import remains unfinished.

The sole Xbox runtime dependency is the clean XboxXBERecomp checkout pinned in
`dependencies.lock.json`. `scripts/prepare_xbe_*.py` applies title adaptations in
the explicit order in `scripts/build_xbe_game_trial.py`. The root CMake target
invokes that guarded builder; it does not build the retired runtime.

With qualified inputs present, close the game, set `UCRR_XBE_RELEASE=1`, clear
`UCRR_XBE_BUILD_ONLY`, and run
`scripts/run_animation_resume.ps1 -Script scripts/build_xbe_game_trial.py`.
If the workstation's default zlib DLL path is unavailable, set `UCRR_ZLIB_DLL`
to the verified runtime DLL used by the local installation. Restore the previous
environment afterward. The driver builds the optimized runtime and executes its
production-derived fixtures. Repeated preparation is checked separately.
Development diagnostics are compiled out; bounded failure reports, setup
progress, saves and normal shader caches remain available.

The entity-capacity additions use `prepare_xbe_entity_registry.py` and
`prepare_xbe_entity_pool.py`, following the entity-record and parent-validation
preparers. Reversal runs before the earlier generated-source pins are checked.
`prepare_xbe_entity_capacity.py` extracts the final emitted lifecycle and body
attachment routines for combined checks. The normal pipeline requires
`test_entity_parent`, `test_entity_registry`, `test_entity_pool`,
`test_entity_capacity` and `test_entity_body`; a build-only run is not release
verification. Keep the registry-generation pins, pool instruction pins and
original-failure witnesses with their corresponding helpers.

`launcher/burner-xbe/package.py` requires a verified runtime SHA-256 and fresh
output directory. Compile Setup/Play with its `build.ps1`, run all setup tests
and `test_setup.py --package`, then audit the exact allowlist. Linux adds
`launcher/burner-linux/burner.sh` and its README to the identical shared payload.
Keep Windows as `Windows-x64.zip` and Linux/Proton as `Linux-Proton.tar.gz`;
the published updater selects those exact suffixes. Preserve executable mode
0755 for `burner.sh`. Full instructions are Markdown under `tools/`; retain
`README.txt`, `assets/README.txt` and `README-linux.txt` as compatibility pointers.
Test the published 0.20 updater against both final archives, including a native
Linux replay for permissions and unchanged user state.

Use the same Steam shortcut and prefix for setup/play. The terminal wrapper is
an advanced direct-Proton route; it does not provide Steam's Linux Runtime.

The source archive contains no ISO, XBE, original fonts/textures/movies/audio,
generated title code, saves, diagnostic captures or compiled runtime. Approved
replacement HD UI images and the validated portable shader seed are the explicit
binary exceptions. The seed is not a GPU driver cache. Optional co-op, Spanish
and voice resources are derived locally from the player's verified discs.
The body-shield recipe likewise derives its models from authenticated local
`shield.lst`, `40-01.ent` and `12-01.ent`; neither the source archive nor the
binary packages contain those original models or the derived model archive.

Version 0.21 reuses the capacity-fix runtime
`df46572720fd9cdaf1d40b49af19de09ff382d9330c79aaef03f285966e74fb1`.
Its complete normal release build passed 241 stages, including 193 test steps.
The source export checks runtime inputs against that build's frozen source
receipt and records release-only launcher/documentation changes separately.
This audit does not establish a fresh-disc clean bootstrap.

Focused fixtures register and retire 3,075 entities across all four grown world
lists, exercise pool growth through 4,097 nodes, and verify successful character
body attachment with the original registry full. They preserve generation-ID
uniqueness, lookup identity, ordering, reuse and cleanup. The tests execute
emitted routines with explicit heap/factory boundaries; they are not complete
NPC-constructor or campaign replays. The same four new capacity fixtures passed
on native Linux. Existing renderer, co-op, input and audio regressions remain
part of the normal pipeline; GPU checks distinguish hardware from WARP.

The tester confirmed that disabling Forever ragdolls prevented the reported
crash. Actual live entity occupancy was not captured, and the new candidate's
replay with Forever enabled remains unconfirmed. Live Windows long-session,
mission-transition/save-load and Proton/Steam Deck gameplay acceptance remain
separate from offline checks. Native Linux fixture results do not establish
Proton video, audio, controls or gameplay compatibility.
