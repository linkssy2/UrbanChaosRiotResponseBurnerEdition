# Code map

Public folders keep the existing include paths intact. Historical builds and captures
are outside this source snapshot; they are not alternative release implementations.

| Area | Location | Editing notes |
| --- | --- | --- |
| ISO setup and direct launch | `launcher/burner/` | Both EXEs share `import_game.py`; stage imports before replacement, never put saves under installed game data. |
| Release identity | `launcher/burner/release.json` | Single version, package path and tested-runtime hash used by packaging/build checks. |
| PC window/input entry | `src/main.c` | Preserve focus, mouse capture and guest thread state across menu transitions. |
| Native menus | `src/hybrid_frontend_host.h`, `src/hybrid_frontend_guest.inc` | Host draws menus; guest bridge observes authoritative profile/mission state. |
| Graphics settings and input | `src/xbox_overlay.h`, `src/xbox_controls_layout.h` | Settings changes must preserve Apply/discard semantics and renderer lifecycle. |
| Guest dispatch | `src/recomp_manual.c`, `src/recomp/` | Guest addresses are contracts, not host pointers. Stack, flags and calling conventions matter. |
| Lighting and rendering | `src/hybrid_*`, `src/migration_graphics_*` | Preserve actual GPU fence completion; do not turn an unsupported wait into success. |
| Physics and retained bodies | `src/migration_physics_*`, `src/character_controller_pool.c` | Pool overflow objects have paired allocation/release. The historical controller-pool name also covers other shared pools. |
| Effects and animation | `src/migration_particles_*`, `src/hybrid_particle_*`, `src/migration_animation_*` | Keep simulation order and original ownership separate from render submission. |
| Native engine support | `native/` | Includes both shared components and standalone-native experiments; presence does not imply enabled in this release. |
| Upstream modifications | `dependency-patches/`, `dependency-overrides/` | Apply against the exact recorded xboxrecomp revision. Preserve upstream notices. |

Function comments focus on ownership, validation and behavior that is unsafe to infer
from names. The dense guest-ABI routines are not cosmetically rewritten for 0.01:
such edits need differential checks and another gameplay test.
