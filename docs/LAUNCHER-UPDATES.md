# Launcher updates

Burner Edition 0.09 adds manual official-release updates, adapted from the
neighboring Darkwatch launcher's `release_check`, `release_download`,
`release_update` and independent Windows recovery bootstrap. No Darkwatch game
assets or runtime code are involved.

Windows: **Manage > Check for updates**, review the offered version, confirm.
The launcher downloads in the background with Cancel available, verifies the
archive, exits for its independent updater, then reopens. Play updates the
existing selected runtime through the normal installation worker.

Linux/Proton: stop the game and Steam setup shortcut, then run
`./burner.sh check-updates` and `./burner.sh update` in the package folder.
Native Python 3.10+ handles package replacement to preserve Unix permissions;
Steam continues to use the same bundled Windows worker and prefix for playing.
This follows Darkwatch's native update route, not a new native game port.

Only newer, published, non-prerelease assets with the exact project, tag and
platform naming are selected. Requests are anonymous and bounded. Downloads
require GitHub's SHA-256 digest, HTTPS on GitHub's release hosts, exact length,
safe archive paths and a complete package manifest. Symlinks, device paths,
duplicate/case-colliding paths and installed user data are rejected. Offline
checks report unavailable; they do not prevent playing or claim up-to-date.

`package-manifest.json` records every shared managed file, including the
launcher and HTTPS-capable embedded Python. Linux adds two wrapper files,
authenticated by the full archive digest. Both packages share the same manifest.
`build.ps1` writes it after both launchers compile; the final package audit checks
it against the complete allowlist. Never ship personal data in this manifest.

The fixed `.launcher-update` directory holds a verified inactive tree, independent
helper and backup. Activation verifies all old/new/backup hashes and free space
before writing. Each replacement is atomic; a journal allows interruption
recovery. Windows waits for launcher/worker processes to exit and reacquires the
setup locks. Linux checks running workers/game processes and uses native locks.
Recovery only touches listed managed files. GameData, installation.json, source
ISOs, external installations, settings, saves, caches and optional installed
packs are outside the operation set. Unrecognized destination collisions fail.

Windows entry points recover before invoking the normal embedded interpreter.
Linux shell entry points use the saved helper; Steam reports recovery guidance
if an update is pending. Run `./burner.sh recover-update` after an interruption.
Do not delete its backup until recovery finishes. A later successful worker
launch removes completed transaction files. Linux interpreter-changing updates
require a complete manual package replacement until Steam has an independent
interpreter recovery entry point. No auto-update on startup, prerelease channel,
or separate HD download is added.

Checks: release selection/network and archive fixtures, interruption at every
replacement, rollback and full-disk failure, user-state preservation, locking,
native Linux file modes and shell routing, and a real Windows independent-helper
handoff/restart with synthetic content. Native Linux fixtures are not Proton
gameplay or a live Steam update test. An update from a future published release
cannot be accepted before that release exists.
