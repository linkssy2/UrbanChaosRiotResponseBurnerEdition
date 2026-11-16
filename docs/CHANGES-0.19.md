# Burner Edition 0.19

- Add a **Download and install HD textures** action under Manage > Add-ons.
  It downloads the existing official 0.09 pack, verifies its pinned size and
  SHA-256, then uses the established installer. Local-ZIP installation remains
  available. The separate HD pack is not bundled in either game package.
- Add Off, Low, Medium and High dynamic-lighting coverage presets. Off retains
  gun, hands and flashlight lighting; Low reduces character lighting and disables
  scenery point lights; Medium reduces scenery light range and character
  lighting; High keeps full coverage. Existing custom combinations remain saved
  until a preset is chosen.
- Remove the separate lighting-resolution control. Internal render resolution
  remains a separate setting.
- Reduce repeated CPU work in geometry/index preparation, lighting transforms
  and texture update tracking while retaining effects and particle density.
  Performance depends on the scene, hardware and settings; there is no general
  FPS-gain guarantee.
- Add separate Guns and Melee / throwables wheels with the existing HUD weapon
  portraits, independent bindings for each player, and smaller translucent
  panels without faint internal seams. Taser and chainsaw use the second wheel;
  the grenade launcher uses Guns.
- Blur the background and run single-player simulation at 20% speed while a
  weapon wheel is open. Audio, input and menu clocks keep their normal rate.
  Co-op retains normal time and independent wheels for both players.
- Add the approved compact ember-arrow pointer, with cursor dimensions reduced
  by 25%.

Hold a wheel control, point with the mouse or physical right stick, and release
to equip. Return to the centre before releasing to cancel. Both wheel controls
held together cancel selection until both are released. Fresh keyboard defaults
are **Tab** for Guns and **G** for Melee / throwables. Existing assignments are
preserved; missing defaults are added only when their keys are unused.
Controller wheel bindings start unbound and can be assigned in Controls.

The No HUD scope fix, dead-enemy blood pools with ledge clipping, TV electrocution
audio correction, updated cover and previous gameplay features remain included.
Knocked-out enemies do not create blood pools. Missions start normally; no hotel
test spawn is included. Manage > DLC / Prototypes remains visible but unavailable.
Prototype integration is paused. Original and prototype game files, test saves
and private test scenes are not bundled.

Windows and Linux/Proton use the same runtime and shared resources. Linux runs
the Windows runtime through Proton. The existing 0.09 optional HD texture pack
is unchanged and does not need reinstalling. Keep the compatibility TXT files
with the package; full setup and update instructions remain in the Markdown
README files.

The user accepted the separate wheels, compact ember-arrow pointer and removal
of faint wheel seams on Windows. Prior Canals lighting and rendering-preparation
checks provide scene-specific evidence. Automated native, input, UI, graphics,
installer and package checks are recorded separately. Live Proton/Steam Deck
and a full campaign pass remain unverified. Co-op remains experimental; HD HUD
remains a work in progress with the original HUD available.

Source downloads are inspection/contribution snapshots. Clean bootstrapping
still requires the private title inputs described in `docs/BUILDING.md`.
