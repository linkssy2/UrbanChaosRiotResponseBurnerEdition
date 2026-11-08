# Urban Chaos: Riot Response — Burner Edition 0.15

Changes since the published 0.14 source snapshot:

- **Rendering performance:** reduce redundant render-state and render-target
  work, accelerate qualified lighting bounds queries, reuse synchronous
  downsample storage, and capture co-op scratch stacks only when guest calls
  require them. Resource lifetime, queued-state ownership and readback ordering
  checks remain. These changes do not lower resolution, lighting, effect,
  geometry or texture settings.
- **Menus and input:** fresh mouse/keyboard or controller activity owns ordinary
  menus, preventing simultaneous highlights. Brief rebinding taps are retained
  between input polls. Full-screen menu clipping no longer exposes the original
  menu at the corners during transitions.
- **View Controls:** replace the old controller diagram with a framed,
  scrollable reference to current bindings. Mouse scrolling, clickable controls,
  right-click Back and player-specific co-op ownership use the existing menu
  framework.
- **Gameplay blur:** add a saved, default-On Graphics option shared by
  single-player and both co-op views. Apply takes effect immediately for aiming,
  firing, hit-feedback and downed blur. Disabling it leaves glow, authored
  particles, downed warnings and gameplay rules intact.
- **Environmental audio:** restore the title's environmental and directional
  sends through the pinned runtime's DSP processors and apply authored per-voice
  low-pass filters. Preserve deferred settings, channel headroom and dry/wet
  routing. This is not a claim of waveform-identical Xbox HRTF or DSP behavior.
- **Health packs:** spend a stored self-heal at its injection event, so an
  earlier cutscene interruption retains the pack. Protect the original
  maximum-health query's full scratch-stack footprint.
- **Body dragging:** extend the existing interaction to knocked-out enemies,
  preserving their health, arrest/recovery state and current physics ownership.
- **Graffiti:** make both lettering and its dark outline translucent for the
  exact surveyed graffiti identities, including original and existing HD
  replacements. Preserve their color, cutout coverage and blending behavior.
- **Support:** include the bug/performance issue template and Markdown release
  instructions. Legacy packaged text-file entry points are retained where the
  published updater requires their names.

Windows and experimental Linux/Proton share the same runtime and asset recipes.
The existing **0.09 HD texture pack remains compatible**; no replacement texture
download or reinstall is required.

The Windows user confirmed Gameplay blur Off removes blur and accepted the menu
transition correction. A separate Windows check covered View Controls entry,
bindings, scrolling and right-click Back. Automated audio, healing, dragging,
rendering and input checks remain distinct from live acceptance. The retained
Canals benchmark evidence applies to its measured settings and build pair, not
all hardware or Steam Deck. Live Linux/Proton gameplay, full-campaign behavior,
environmental-audio comparison and all co-op controller paths are not established
by these checks. Co-op remains experimental. Clean source bootstrapping still
needs the qualified private title inputs described in `BUILDING.md`.
