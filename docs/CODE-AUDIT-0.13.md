# 0.13 maintained-code audit

The release source inventory supplied 357 implementation files (41,743 lines)
and 163 test files (34,454 lines) before cleanup. Four review areas covered the
launcher and asset preparation, gameplay/input/menus, rendering/audio, and build
integration. Implementation files were read in full; tests were reviewed where
connected to a finding and exercised by the maintained checks. This is not a
claim that every test, generated Xbox instruction or upstream dependency was
individually hand-reviewed, or that no further optimization is possible.

## Corrections

- Health-bar animation history is now independent for each validated co-op
  player. Alternating HUD draws previously reset one shared history.
- Controls Defaults now restores Walk mode and the selected device's Walk and
  Toggle HUD bindings. The other device and player's settings are preserved.
- Game crash reports use the launcher's checked atomic writer. Linked error
  files cannot overwrite unrelated data, and a reporting failure no longer
  hides the original game exit code.

## Cleanup

- Removed the empty geometry-cache wrapper. Vertex/index snapshots still use
  the existing upload arena, synchronization and 16-byte index alignment.
- Archived the rejected brightness-readback experiment and its two orphaned
  fixtures. Its small generated-hook removal routine remains for old trees.
- Removed unused co-op state, a preparation dictionary and an unused import.
- Updated the documented Support path for optional logging.

The before/after health and Controls fixtures reproduce their failures. Launcher
checks cover linked files and failed writes; renderer checks compare actual draw
snapshots after source mutation and command-list reset. Release verification
rebuilds the runtime and checks repeated preparation, package inventories and
upgrade preservation. Exact coverage hashes and detailed audit receipts are in
`build/code-audit-20261105`; final package results are in `build/release-0.13` and
`publication/0.13/release-manifest.json`.

Logging stays off by default. Bounded user-enabled support logs and small failure
reports are intentional; developer captures, profiling and dump hotkeys are
excluded from the release runtime. Useful diagnostic source stays in the source
archive for maintenance. No FPS gain is claimed for removing inactive code.

Follow-up user testing found that the original Controller Info screen was missing
from the menu/input visibility checks. The HUD now respects original screen
identities as well as replacement menus, and the diagram registers as a menu for
right-click Back. The new regression checks cover stale health-pack snapshots,
single-player and both co-op pause owners, transitions, and return to gameplay.
Evidence is recorded separately in `build/controller-info-20261105`.

Windows and Linux/Proton packages share the same runtime and source revision.
Offline native-Linux setup/update tests are separate from Windows checks; they
do not establish live Proton gameplay. This audit does not launch the game or
establish fresh gameplay, frame-time or physical surround-output acceptance.
