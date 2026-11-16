# Burner Edition 0.19 build status

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

With qualified inputs present, close the game, set `UCRR_XBE_RELEASE=1` and run
`scripts/run_animation_resume.ps1 -Script scripts/build_xbe_game_trial.py`.
Clear that environment variable afterward. The driver builds the optimized
runtime and executes its production-derived fixtures. Repeated preparation is
checked separately. Development diagnostics are compiled out; bounded failure
reports, setup progress, saves and normal shader caches remain available.

`launcher/burner-xbe/package.py` requires a verified runtime SHA-256 and fresh
output directory. Compile Setup/Play with its `build.ps1`, run all setup tests
and `test_setup.py --package`, then audit the exact allowlist. Linux adds
`launcher/burner-linux/burner.sh` and its README to the identical shared payload.
Keep Windows as `Windows-x64.zip` and Linux/Proton as `Linux-Proton.tar.gz`;
the published updater selects those exact suffixes. Preserve executable mode
0755 for `burner.sh`. Full instructions are Markdown under `tools/`; retain
`README.txt`, `assets/README.txt` and `README-linux.txt` as compatibility pointers.
Test the published 0.18 updater against both final archives, including a native
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

The current feature fixtures cover lighting tiers, exact receiver-inverse and
dirty-epoch reuse, vertex/strip equivalence, separate per-player wheels, scoped
single-player time, background blur, existing HUD portrait reuse, translucent
wedge geometry, and the DPI-aware menu cursor. GPU checks distinguish hardware
from WARP. Private profiling traces/toggles and rejected experiments are excluded
from the release; diagnostic source may remain where release guards remove it.

Native Linux wrapper tests do not establish Proton video, audio, controls or
gameplay. Automated checks and user acceptance of particular Windows fixes are
recorded separately from full-campaign and platform coverage.

This launcher-only revision reuses the qualified v0.19 game runtime unchanged.
Its separate source freeze records updated launcher, tests and documentation;
the original runtime-build freeze and qualification remain referenced by hash.
The HD download/install fixtures exercise the pinned archive and failure paths.
A real Windows Python worker-function download/install was also checked; this
is separate from a WinForms click, gameplay, or live Proton acceptance.
