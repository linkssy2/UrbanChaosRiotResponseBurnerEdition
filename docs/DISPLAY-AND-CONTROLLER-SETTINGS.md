# Display and controller settings

## 16:10

Graphics offers Automatic, 4:3, 16:10, 16:9, 21:9 and 32:9. The new 16:10
choice uses saved ID 5, leaving existing profiles unchanged. Automatic already
preserves the output's actual aspect ratio. The same 1280x800, 1920x1200 and
2560x1600 windowed presets used by Darkwatch are always listed; exclusive
fullscreen still requires a mode supported by the display.

Urban Chaos already carries this ratio through the root viewport, split-screen
projection, menus, HUD, first-person rendering and cinematics. Darkwatch's extra
vertical menu fitting is not needed for Urban's original 640x480 menu bands.
FMV modes retain their existing Fill, Original aspect ratio and Fill stretch behavior;
choosing a different display ratio does not silently change the movie mode.

Graphics groups display options, rendering quality, and camera/HUD controls in
one scrolling list. Controls puts input settings before combat/interaction,
movement, teammate orders and menu actions. Display ordering does not change
saved action IDs, bindings or profile ownership.
Controller vibration, stick inversion, auto-centring, stick layout and stick
sensitivity appear only on the controller pages. Crouch mode stays beside the
keyboard Crouch binding because it applies to both input devices. Keyboard
Defaults preserves the hidden controller settings.

## Controller deadzones (issue 24)

The current Xbox-controller report concerns stick response, not movement
acceleration. Movement inertia and stick deadzone are separate settings. Sprint
remains out of scope.

Each controller page has left- and right-stick deadzone sliders. Numeric values
cover 0-30%; 0% removes the inner threshold. The endpoint labeled Original
preserves the original game's response, including any previous additional
deadzone configured in an older profile. It is not a 31% threshold. Defaulting
to Original avoids silently changing existing controller behavior.

The settings apply to the selected player and physical stick, including Legacy
thumbstick layouts. They do not alter mouse aiming or menu navigation. Apply,
Discard and Defaults use the existing profile save transaction. A 0% setting
can expose a controller's physical drift.

The maintained path is SDL raw axes, native normalization at 22DFA0, descriptor
evaluation at 22EDF0, then WorldInput. Calibration set by 22FB00 uses the pinned
40% inner and 85% outer constants at 4FD08C. The new hook changes only the inner
response of qualified gameplay stick descriptors and retains the original outer
endpoint, clamp/accumulation flags and button/edge processing. Remapped axes use
their physical source's setting. Menus, keyboard contributions, focus loss and
unregistered devices retain original processing.

The first test candidate admitted connected-stick drift while mouse/keyboard
owned Player 1: only axes with a nonzero keyboard contribution were excluded.
The correction excludes all Player 1 controller adjustments during mouse/keyboard
ownership, including ownership changes between a pad sample and its native
evaluation. Player 2's controller remains independent. The legacy host axis
filter stays intact for the original fallback; configured controller response
uses the cached physical sample so the two deadzones are not stacked.
Existing physical button and deliberate stick activity still switches back to
controller ownership. Once switched, a 0% setting accepts even the smallest
stick sample; the device-switch threshold is not applied again to gameplay.

## Verification

Private paired candidate and receipts: `build/issue24-1610-20261106`.
Input isolation and controller-page follow-up: `build/controller-isolation-20261106`.
Both Windows and Linux/Proton packages use the same runtime and shared assets.
The maintained build exercises settings persistence, original input processing,
per-player ownership, menu mouse/controller input, 16:10 layouts and all
catalogued FMV aspect modes. The earlier latched ragdoll-drag feature is retained.
The input-isolation fixture compares Original/0/15/30% with mouse movement,
WASD/opposing keys, legacy filters, connected/disconnected controllers and both
player counts. Removing the consumption-time ownership guard makes its actual
raw-mouse takeover assertion fail. Keyboard Defaults/Apply/Discard preservation
and controller-only page membership have dedicated menu checks.

Automated checks do not establish real-controller feel, a full campaign visual
pass, or Proton live compatibility. Record those separately after testing.
