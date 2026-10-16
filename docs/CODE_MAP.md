# Code map

The 0.03 PC port uses the XboxXBERecomp Direct3D12 runtime. The maintained
integration lives in `scripts/`; generated title/runtime overlays stay private
under `build/`. See [build status](BUILDING.md) before attempting a clean build.

| Area | Maintained source | Editing notes |
| --- | --- | --- |
| Build order and checks | `scripts/build_xbe_game_trial.py` | Stages checked overlays against the pinned SDK, then builds and tests. |
| Original function hooks and runtime overlays | `scripts/prepare_xbe_*.py` | Expected source shapes and original-byte checks must fail on incompatible inputs. |
| PC menus | `scripts/xbe_trial_graphics_ui.cpp`, `xbe_frontend_guest.inc` | Keep original profile/mission state authoritative; prevent menu click-through. |
| Input and rebinding | `scripts/xbe_trial_menu_input.cpp`, `xbe_controls_*.inc` | Preserve focus/capture and Apply/discard semantics. |
| Graphics and fog | `scripts/prepare_xbe_graphics_settings.py`, `prepare_xbe_smooth_fog.py`, `prepare_xbe_msaa.py` | Target lifetime, queued state and depth sampling must remain consistent. |
| Prompt drawing | `scripts/prepare_xbe_pc_prompts.py`, `prepare_xbe_prompt_handoff.py` | Prompt metadata must survive the actual render queue; Reload and Dialogue Skip are distinct contexts. |
| Audio | `scripts/prepare_xbe_audio_*.py`, `xbe_audio_loop.*` | Preserve sample rate, source ownership, deferred settings and event ordering. |
| Languages | `scripts/xbe_language.*`, `xbe_language_ui.inc`, `xbe_frontend_text.h` | Select supplied original banks on restart; convert host copies from CP1252 to UTF-8. |
| Body retention | `scripts/xbe_ragdoll_settings.cpp`, `prepare_xbe_ragdoll_settings.py` | Keep original object ownership and cleanup safe. |
| Quiet release | `scripts/prepare_xbe_release*.py` | Remove diagnostics without changing gameplay; validate installed asset identity. |
| ISO setup and direct Play | `launcher/burner-xbe/` | Validate/stage imports before replacement and preserve settings/saves. |
| Tests | `scripts/test_xbe_*` and launcher `test_setup.py` | Some fixtures require private generated title/resource inputs. |
| Upstream identity/notices | `dependencies.lock.json`, `licenses/` | XboxXBERecomp by TGP482 is the current runtime. |

`src/`, `native/`, `shaders/`, `dependency-patches/xboxrecomp.patch` and the root
CMake project preserve earlier work. A few reviewed helpers are reused by the
current preparation scripts; their presence does not mean every historical
native experiment is active in 0.03. Existing file locations are retained to
avoid changing include paths or introducing gameplay changes into a tested
release. Comments focus on ownership and behavior that cannot be inferred from
function names.
