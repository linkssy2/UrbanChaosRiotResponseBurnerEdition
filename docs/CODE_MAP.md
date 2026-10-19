# Code map

The 0.05 PC port uses the XboxXBERecomp Direct3D 12 runtime. Maintained
integration lives in `scripts/`; generated title/runtime overlays stay private
under `build/`. Read [build status](BUILDING.md) before attempting a clean build.

| Area | Maintained source | Editing notes |
| --- | --- | --- |
| Build and checks | `scripts/build_xbe_game_trial.py` | Stages checked overlays against the pinned SDK, then builds and tests. |
| Original function hooks | `scripts/prepare_xbe_*.py` | Preserve expected source shapes and original-byte admission checks. |
| PC menus | `scripts/xbe_trial_graphics_ui.cpp`, `xbe_frontend_guest.inc`, `xbe_menu_style.h` | Keep original profile/mission state authoritative and prevent menu click-through. |
| Input and rebinding | `scripts/xbe_trial_menu_input.cpp`, `xbe_controls_*.inc` | Preserve device switching, focus/capture and Apply/discard behavior. |
| Graphics, FOV and crosshair | `scripts/xbe_graphics_settings.*`, `prepare_xbe_view_layout.py`, `prepare_xbe_hd_ui.py` | Publish accepted settings together; leave original size at 100%. |
| HD UI | `scripts/xbe_hd_ui.*`, `xbe_hd_ui_restored.h`, `xbe_hd_ui_reconstruct.h`, `prepare_xbe_hd_ui.py`, `artwork/hd-ui/manifest.json` | Restore local glyphs once; load hash-pinned artwork and preserve source bounds/resource lifetimes. |
| Host fonts and symbols | `scripts/xbe_frontend_fonts.*`, `prepare_xbe_pc_prompts.py`, `prepare_xbe_prompt_handoff.py` | Keep glyph metrics, controller families and queued prompt context consistent. |
| Vertex assembly | `scripts/prepare_xbe_vertex_costs.py`, `xbe_packed_vertices.inc` | Preserve authored values, stream bounds, index order and fallback behavior. |
| Draw/resource reuse | `scripts/prepare_xbe_draw_reuse.py`, `xbe_sampler_inputs.h`, `prepare_xbe_frame_reuse.py` | Reuse only exact inputs within valid descriptor/resource lifetimes. |
| Visibility queries | `scripts/prepare_xbe_visibility.py`, `test_xbe_query_resolves*.cpp` | Resolve completed ranges before submission and ring-slot reuse; preserve report fences. |
| Fog and MSAA | `scripts/prepare_xbe_smooth_fog.py`, `prepare_xbe_msaa.py` | Keep target lifetime, queued state and depth sampling consistent. |
| Audio | `scripts/prepare_xbe_audio_*.py`, `xbe_audio_loop.*` | Preserve sample rate, source ownership, deferred settings and event order. |
| Languages | `scripts/xbe_language.*`, `xbe_language_ui.inc`, `xbe_frontend_text.h` | Select supplied original banks on restart; convert host strings to UTF-8. |
| Body retention | `scripts/xbe_ragdoll_settings.cpp`, `prepare_xbe_ragdoll_settings.py` | Preserve original object ownership and safe cleanup. |
| Quiet release | `scripts/prepare_xbe_release*.py` | Disable diagnostics without changing gameplay; validate imported asset identity. |
| Setup and direct Play | `launcher/burner-xbe/` | Validate staged imports into portable GameData; first import copies compatible legacy state without changing the source. |
| Tests | `scripts/test_xbe_*`, launcher `test_setup.py` | Some fixtures require private generated title/resource inputs. |
| Upstream identity | `dependencies.lock.json`, `licenses/` | XboxXBERecomp by TGP482 is the current runtime. |

The historical `src/`, `native/`, `shaders/`, root CMake project and
`dependency-patches/xboxrecomp.patch` remain in their established locations.
Only selected reviewed helpers are reused by current preparation scripts;
their presence does not mean every historical experiment is active. This avoids
moving include paths or introducing runtime changes during source packaging.
