## Burner Edition 0.05 — Portable setup, HD UI and performance update

**HD HUD is still a work in progress and has known prompt/display issues. Turn HD UI Off to use the original HUD, which works normally.**

Changes since **0.04**:

### HUD and graphics

- Added an optional **HD HUD and menus** setting, including restored weapon/ammo icons, medals and menu pictures, plus sharper fonts and button artwork. It also applies to original controller menus and pause screens; HD UI defaults to Off.
- Added a **world FOV slider**: 40–90 vertical degrees, default 53, preserving the original zoom behavior.
- Added a separate **Weapon FOV slider**: 50–150%, default 100%, adjusting the gun/hands view independently of the world camera.
- Added a **reticle-size slider**: 50–200%, default 100%.
- Improved **16:9 HUD placement** and corrected interaction brackets being shifted away from their target by a second widescreen adjustment.
- Corrected the **Display Orders** hint to show the configured mouse/key binding in keyboard-and-mouse mode.

### Menus and input

- Brought the earlier Burner Edition mouse-menu style to the Direct3D 12 build, including main, pause, Audio and General pages, with matching selection rows and change arrows.
- Replaced the pause **Quit** confirmation with mouse-friendly Yes/No rows while retaining the original warning, sounds and actions.
- Suppressed original menu rows/sprites beneath their mouse replacements so they do not show through during transitions. Controller users retain the original menus; added Graphics and Controls pages remain available.
- Improved short keyboard-tap delivery through original menu updates, cleared stale input at screen/focus changes, and prevented rapid confirm presses from carrying into a different page.
- Integrated the new sliders and HD toggle with mouse, keyboard and controller navigation, Apply, Discard and saved settings.

### Performance and caching

- Improved frame pacing and recovery in busy scenes, with work focused on the **Canals opening and car explosion**. Shared renderer changes apply across levels; frame drops can still occur.
- Corrected vertical-blank waiting and redundant waits during the game's flash/fade display path.
- Made renderer busy checks nonblocking and submitted large draw workloads in bounded batches.
- Batched completed visibility-query results while retaining the original visibility counts and synchronization.
- Reduced vertex/index conversion, allocation and upload overhead; reused unchanged geometry within valid frame lifetimes.
- Reused texture views and sampler inputs, sped up texture-format lookup and render-state comparisons, and removed repeated texture/palette/MSAA metadata work.
- Improved descriptor and upload-buffer lifetime handling when frame storage fills up.
- Added persistent D3D12 pipeline caching, moved shader/cache writes off the drawing path, and improved rejection of stale or incomplete cache entries.
- Reduced repeated audio loop-bound calculations without changing playback behavior.

### Portable installation and release cleanup

- **GameData now stays beside the launchers**, including imported game files, profiles, settings and shader cache. Move the whole folder to keep the installation together.
- First ISO import can copy compatible saves/settings/cache from the previous AppData-based XBE installation without changing the old copy. Existing portable data takes priority; cancelled or failed imports preserve it.
- Direct Play uses this installation's GameData, independent of a shortcut's working directory, instead of silently falling back to AppData.
- Updated the source snapshot, code map and maintenance notes. Release diagnostics/capture hotkeys are disabled; bounded error reporting remains available for failures.

**Install/update:** Extract the complete **Windows x64 ZIP** to a writable folder. Run **Burner Edition Setup.exe** and select your supported Xbox ISO/XISO, then use **Play Burner Edition.exe**. Existing portable users should **copy their GameData folder** beside the new launchers. Back up saves before removing an old installation. Required runtime DLLs are included.

**Content:** No original disc files, Xbox executable, videos or saves are supplied. Your ISO provides the game content. The download includes the approved restored HD replacement artwork and custom Burner Edition artwork.

**Other limits:** Performance varies by scene and hardware; this is not a guarantee of constant 60 FPS. Switch 2 USB remains experimental and hardware-unverified; Bluetooth is unsupported. The source snapshot still requires private title-generation inputs for a complete rebuild, as documented in the build notes.

Powered by **[XboxXBERecomp by TGP482](https://github.com/TGP482/XboxXBERecomp)**, with thanks to **[sp00nz's xboxrecomp](https://github.com/sp00nznet/xboxrecomp)** for the earlier foundation and retained helpers.
