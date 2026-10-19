# Burner Edition: current runtime

The current candidate uses the pinned XboxXBERecomp runtime in
`build/upstream-review-xboxxbe-20261014`. The earlier `src/` runtime remains for
historical builds; it is not the current D3D12 executable.

## Where to edit

| Area | Maintained source |
| --- | --- |
| Build and verification order | `scripts/build_xbe_game_trial.py` |
| Original function hooks and dependency overlays | `scripts/prepare_xbe_*.py` |
| PC menu drawing | `scripts/xbe_trial_graphics_ui.cpp`, `xbe_menu_style.h` |
| Menu state and original callbacks | `scripts/xbe_trial_guest_input.cpp`, `xbe_frontend_guest.inc` |
| Rebinding | `scripts/xbe_controls_*.inc`, `xbe_controls.h` |
| Languages | `scripts/xbe_language.cpp`, `xbe_language_ui.inc` |
| Prompt classification and renderer handoff | `scripts/prepare_xbe_pc_prompts.py`, `prepare_xbe_prompt_handoff.py` |
| Optional HD UI art and original texture matching | `scripts/xbe_hd_ui.cpp`, `xbe_hd_ui_filter.h`, `prepare_xbe_hd_ui.py` |
| Field of view, widescreen HUD and reticle size | `scripts/xbe_view_layout.cpp`, `prepare_xbe_view_layout.py` |
| Original/HD host menu fonts and button atlas | `scripts/xbe_frontend_fonts.cpp` |
| Quiet release policy | `scripts/prepare_xbe_release.py` |
| Installed game admission | `scripts/prepare_xbe_release_assets.py` |
| ISO import, Setup and direct Play entry points | `launcher/burner-xbe/` |

Generated files under `build/xbe-full-integration-20261014` are private outputs.
Change a maintained source or preparation script, not just its generated copy.
Preparers pin the original bytes or expected source shape so incompatible updates
fail before they can silently apply the wrong patch. Keep the upstream pin until
its replacements pass the same checks.

## Important boundaries

- `gpu::State` additions must be inside the ranges copied by the render queue.
  The prompt handoff regression executes the real transport and an old-layout
  negative witness; a direct renderer test alone misses that failure.
- Language changes select an existing original bank at the next startup. Do not
  switch loaded banks in place or change the video region to choose a language.
  Original strings use CP1252; only host UI copies are converted to UTF-8.
- Release disables routine logging, captures and diagnostic timing at compile
  time. Fatal errors can still reach the launcher's bounded failure report.
  Keep frame pacing, shader caching and normal FPS display independent of tracing.
- Setup verifies imported files locally. The release runtime only accepts its
  explicit extracted directory and the supported executable hashes. Never add a
  permissive fallback or package a developer `assets` directory.
- New runtime settings and `hdd` saves live under the user's `xbe-runtime`
  directory. Updating the executable must not replace those files. The older
  runtime's saves are separate and must not be silently converted or deleted.

## Build and package

Close the game first. Run the existing heavy-build wrapper with
`UCRR_XBE_RELEASE=1` and `scripts/build_xbe_game_trial.py`; clear that environment
setting afterward. This stages the overlays, builds the optimized executable,
and runs the production-contract fixtures. The build report and private symbols
remain beside the build, never in the release ZIP.

Use `launcher/burner-xbe/package.py` with the exact verified runtime SHA256 and a
fresh output directory, compile both launchers with its `build.ps1`, then run the
package audit and `test_setup.py --package`. Its explicit allowlist excludes game
assets, saves, caches and diagnostics. Never archive the workspace as a release.

Offline tests prove their stated contracts, not a full campaign playthrough.
Keep live menu, language/audio, gameplay and performance acceptance separate in
the candidate's verification notes.

## HD HUD and menus

The Graphics option `hd_ui` defaults to Off and uses the existing Apply/Discard
transaction. It changes artwork, not layout, menu navigation, input bindings,
original text metrics or the controller/mouse menu selection policy.

At startup, `uc_hd_ui` reads named UI assets from the user's installed archives.
Small icons, HUD textures and fonts use 4x reconstruction. Approved restored PNGs
in `artwork/hd-ui/manifest.json` replace matching weapon/ammo illustrations,
medals and menu photos at 4x. Unreplaced large panels retain the 2x filtered path.
Font contours and inner outlines use validated original glyph cells; action
symbols keep their original shapes. Soft health-bar masks and the animated
health geometry retain their authored behavior.

The manifest pins both the complete original texture payload and replacement
PNG by SHA256. The build generates `xbe_hd_ui_artwork.h` and copies only listed
PNGs beside the executable. The packager uses that same manifest. Setup verifies
the artwork before installing it; original game files still come from the ISO.
Do not include private original-image exports in either release archive.

Restoration runs once at startup, with a 512 MiB decoded UI/source budget.
Transparent sprites map to their original content bounds; arbitrary recoloring
or global resizing must not change their apparent size. Invalid or missing
replacement files fall back to prepared original art. The release check requires
every approved manifest record to load and bind, so a fallback cannot silently
pass qualification.

Original controller menus and HUD use the shared D3D12 texture path. Matching
requires dimensions, format and every compressed mip byte to agree with a named
UI asset. Prepared textures have separate GPU storage; authored coordinates and
shader constants remain unchanged. Matching runs on original content changes,
not every draw. Both variants survive toggles, and replaced resources retire with
their rendering frame. Unsupported or missing assets retain the original art.

Host menu fonts and icons pack both variants into the existing ImGui atlas once.
PC, PlayStation and Nintendo prompts already use generated high-resolution glyphs;
the HD option also replaces original Xbox button cells. Do not add a global GPU
extension to implement this option: it would bypass existing optimized D3D12 paths.

The maintained build runs the service/filter, renderer lifetime, prompt, settings,
host font and menu fixtures, plus synthetic D3D12 raster cases on hardware and WARP.
These checks cover integration contracts; live appearance and frame pacing remain
separate acceptance checks.

## View and reticle settings

Graphics shares one styled slider implementation for vertical FOV (40–90 degrees,
53 by default) and reticle size (50–200%, 100% by default). Mouse dragging, keyboard
arrows and controller navigation edit the same draft; Apply saves and publishes it.
Discard and a failed save leave the committed view settings intact.

Reticle scaling is limited to the original weapon crosshair draw subtree and its
final sprite quads, with pinned original bytes and caller/class checks. It scales
each quad about its authored center, preserving spread, visibility and fade inputs.
Scope masks, brackets, prompts and unrelated HUD elements keep their geometry.
The temporary coordinates are restored after drawing; 100% uses the previous path.

## Contextual button prompts

`xbe_prompt_context.cpp` tags each original queued glyph by its authored text key.
Interaction/Skip uses the configured secondary A key; `HUD-Help-40` uses the
Orders binding for its D-pad glyph. Keep generic menu-navigation symbols separate.
The tag travels with the renderer's copied state, so delayed draws cannot borrow
another prompt's binding. Each atlas variant has its own cached GPU resource.
HD artwork is applied before generated keycaps; it must not restore a replaced
controller symbol. Controller mode retains its original/remapped symbols.
