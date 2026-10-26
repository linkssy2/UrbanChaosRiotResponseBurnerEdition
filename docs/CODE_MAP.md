# Code map

The 0.08 PC port uses XboxXBERecomp's Direct3D 12 runtime. Maintained integration
lives in `scripts/`; generated title/runtime output stays private under `build/`.
Read [BUILDING.md](BUILDING.md) before attempting a clean build.

| Area | Maintained source | Editing contract |
| --- | --- | --- |
| Build and verification | `scripts/build_xbe_game_trial.py`, `run_animation_resume.ps1` | Preserve checked preparation order and serialized builds. |
| Original-function hooks | `scripts/prepare_xbe_*.py` | Keep original-byte admission checks, source pins and exact calling conventions. |
| PC menus and co-op selector | `scripts/xbe_trial_graphics_ui.cpp`, `xbe_frontend_guest.inc`, `xbe_coop_menu.h` | Normal font startup enables menu availability; gameplay activation still requires selection, readiness and resources. |
| Native character previews | `scripts/xbe_coop_preview*`, `scripts/xbe_coop_preview_assets_checks/` | Cache original body geometry and atlas data; preserve independent Ready input and fallback UI. |
| Preview asset dependencies | `native/archive_reader/`, `texture_asset/`, `model_*`, `material_asset.*`, `texture_sequence.*`, `animation_*` | Active parsers/pose helpers used by current selection UI; do not classify all native code as obsolete. |
| Settings and pause ownership | `scripts/xbe_graphics_settings.*`, `xbe_trial_menu_input.cpp`, `xbe_coop_pause.h`, `xbe_coop_input.h`, `xbe_coop_guest_input.h` | Accepted pause owns editing; whole game pauses. Prevent partner input from changing the menu. |
| Per-player controls | `scripts/xbe_controls_*`, `xbe_coop_controls.*` | Apply/discard and native profile transactions preserve the other player; validate owner lifecycle. |
| Gameplay actors and presentation | `scripts/xbe_coop_guest.cpp`, `xbe_coop_body.*`, `xbe_coop_resources.*`, `xbe_coop_effects.*`, `xbe_coop_cinematics.*` | Preserve independent views, actors and frame state while sharing campaign world/cinematics. |
| Death and revival | `scripts/xbe_coop_death.*`, `xbe_coop_downed_camera.*`, `xbe_coop_downed_blur.*`, `xbe_health_packs.*` | Keep finite inventory, downed timer, body/camera ownership and one-time restored visibility coherent. |
| Orders and contextual hints | `scripts/xbe_coop_orders.*`, `xbe_view_layout.*`, `xbe_prompt_context.*`, `prepare_xbe_prompt_handoff.py` | Validate target/look eligibility, owner queue and per-view placement independently. |
| Personal pickups and rewards | `scripts/xbe_coop_pickups.*`, `xbe_coop_spin.*`, `xbe_coop_rewards.*`, `xbe_coop_ko_rewards.*`, `xbe_coop_collectibles.*` | Grant to collector/instigator once; retain shared mission counts and profile progression. |
| Campaign cooperation | `scripts/xbe_coop_shields.*`, `xbe_coop_progression.*`, `xbe_coop_interactions.*`, `xbe_coop_turrets.*`, `xbe_coop_turret_view.*` | Preserve authored state transitions and shared script semantics; finite supplies remain finite. |
| Derived co-op resources | `scripts/coop_assets.py`, `coop_injection_assets.py`, `coop_turret_assets.py`, `coop_progression_assets.py` | Validate original input hashes; prepare resources locally rather than distributing disc payloads. |
| HUD, FOV and reticle | `scripts/xbe_hd_ui.*`, `xbe_view_layout.*`, `xbe_graphics_settings.*`, `artwork/hd-ui/manifest.json` | Art and layout remain separate; isolate player FOV from shared graphics settings. |
| Arm extensions | `scripts/xbe_arm_mesh.h`, `xbe_arm_mesh_data.h`, `test_xbe_arm_mesh.py` | Match authored meshes; preserve skinning, animation and sleeve texture density for all arm variants. |
| FMV presentation | `scripts/prepare_xbe_fmv_layout.py`, `xbe_fmv_layout.*` | Proportional Fill/4:3 geometry, original UVs and proper clear/lifecycle behavior. |
| Vertex/draw reuse | `scripts/prepare_xbe_vertex_costs.py`, `xbe_packed_vertices.inc`, `prepare_xbe_draw_reuse.py`, `xbe_sampler_inputs.h` | Preserve authored data, resource lifetime, and fallback paths. |
| Visibility, lighting and depth | `scripts/prepare_xbe_visibility.py`, `prepare_xbe_msaa.py`, `xbe_wall_depth.h` | Preserve report fences and target/depth semantics; fixes remain narrowly admitted. |
| Audio | `scripts/prepare_xbe_audio_*.py`, `xbe_audio_loop.*`, `xbe_voice_mode.h` | Preserve source ownership, sample timing, deferred settings and shared output. |
| Language and voice import | `scripts/xbe_language.*`, `xbe_spanish_ui.h`, `launcher/burner-xbe/spanish_*.py`, `ps2_audio.py`, `xbox_adpcm.py` | Bundled text, optional user-supplied PS2 audio, transactional local conversion, no game-audio download. |
| Portable shader seed | `packaging/shader-seed/`, `scripts/build_xbe_shader_seed.py`, `xbe_shader_cache.*` | Exact source/compiler/bytecode identities; preserve learned cache and exclude driver blobs. |
| Quiet release | `scripts/prepare_xbe_release*.py`, `test_xbe_aim_release.cpp` | Compile out diagnostics without changing gameplay; fatal errors remain bounded. |
| Windows setup and Play | `launcher/burner-xbe/` | Atomic import and exact payload validation; both game modes use the normal launcher. |
| Linux/Proton entry | `launcher/burner-linux/` | Same runtime via Steam/Proton with safe quoting, standard prefix symlinks and visible errors. |
| Dependency identity | `dependencies.lock.json`, `licenses/`, `dependency-patches/` | Keep the SDK pin and enumerate only qualified backports. |

The historical root CMake project, unrelated `src/`, native reconstruction and
`shaders/` remain in their established locations. Their presence does not mean
all earlier experiments run in the current title path. Some tests require
private original/generated inputs; fixtures, full compilation and live acceptance
are distinct. See [menu/input maintenance](XBE-MENUS-AND-INPUT.md) and
[runtime maintenance](XBE-RUNTIME-MAINTENANCE.md).
