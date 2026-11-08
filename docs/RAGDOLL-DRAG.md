# Ragdoll limb dragging

Hold the assigned Interact button (F by default) for 0.45 seconds while aiming
at a nearby dead or knocked-out enemy's arm, leg, head or chest. The grab stays latched after releasing the
button. Move or turn to drag the body; hold Interact again to let go. Ordinary
quick Interact presses retain their original behavior. Each co-op player owns
their own grab, and a body can be grabbed by only one player at a time.
If the first attempt misses a limb, keeping Interact held retries at most ten
times per second. A successful grab or drop consumes the hold until release.
The held limb is pulled toward a point 18 cm above its acquired view-relative
offset. This lift is world-up, does not accumulate, and does not teleport the
limb or suspend the rest of the body through the floor.

The feature supports dead NPCs, knocked-out enemies and downed co-op partners. Active NPCs and the
pelvis/lower spine are not grab targets. Acquisition is limited to the nearest native
ray hit within two metres. Walls block acquisition and obstructed grabs release.
Pause, menus, focus loss, death, scripted control locks, revival, body removal,
and excessive separation release the grab. A fresh button release is required
before another grab after cancellation.

Knocked-out enemies retain their health and native behavior. Retail `DF380`
sets the knockout/restraint flag (`actor+0x198`, bit `4`). Positive-health
targets additionally need their own current physics wrapper (`actor+0x118`,
type `48E6E0`), data (`wrapper+0x0c`) and ragdoll handle (`data+0x50`) to match
the grabbed pool entry. Recovery/reset, wrapper disposal or handle replacement
release the grab before another force is applied. The original interaction,
arrest, recovery and corpse-retention paths are not rewritten.

The November 6 knockout follow-up is private, with receipts under
`build/knockout-drag-20261106`. Its fixture executes the hash-pinned original
knockout event, then the production drag logic for both NPC types, both co-op
owners, single-player, limb/head/chest targets, exclusive ownership and stale
body rejection. It verifies that dragging changes neither actor health nor
knockout/AI state. Behavior allocation, ray geometry and native joint/contact
solving remain fixture boundaries. Live knockout dragging, arrest/recovery and
Proton gameplay need separate confirmation.

## Maintained integration

- `scripts/prepare_xbe_ragdoll_drag.py` pins the retail code and installs the
  MainPlayer update hook at D2AB0 plus normal/resume ragdoll hooks at 2A5E7C.
- `scripts/xbe_ragdoll_drag.cpp` observes native held Interact through 567F0
  using physical A (`0x16`), the channel populated by the host Interact binding,
  raycasts through 214230, and applies a bounded, damped force through 2F4FF0.
  Native wake, contacts, joints and the solver remain responsible for movement.
  It does not rewrite body positions or change retention policy.
- The pull combines bounded target travel (6 metres/second) with position-error
  correction (10/second, capped at 5 metres/second), using a 1.4-gain velocity
  correction with exponential response of 90/second. Tracking target travel keeps ordinary walking from outrunning a
  fixed correction-speed cap. Force is derived from the held limb's validated
  inverse mass and native step duration, then capped at 240 metres/second squared.
  Captured grab limbs have masses 10 or 15 in native units, giving force caps of
  2,400 or 3,600. The full captured ragdoll is about 160 mass units: contacts and
  joints share the applied impulse, so the cap does not promise that the entire
  body floats or follows without lag. No constraint, mass or gravity is changed.
- The existing native/overflow pool in `xbe_ragdoll_settings.cpp` resolves exact
  limb membership and detects released/reused handles. Actor identity, model,
  world and player identity are rechecked before applying force.
- `xbe_trial_menu_input.cpp` supplies per-player eligibility and cancellation
  epochs. The feature uses the existing Interact mapping; it adds no fixed-key
  shortcut, new binding, setting, runtime logging or asset dependency.

## Verification boundaries

The build runs the drag fixture, original held-input query checks, native and
overflow pool lifetime checks, and host menu/focus/controller checks. The drag
fixture executes the original player/action query, wake and force integration;
it checks that A/Interact acquires a limb and X/Cycle Firearm does not, for both
co-op owners and single-player. Response checks cover 30/60/120/240/360 FPS,
several limb masses, a fixed world-up lift while pitching the camera, bounded
acceleration, and conservative 4x/16x coupled-mass trajectories. Abrupt 4/6 m/s
camera travel, stopping, and a 90-degree turn are checked at every tested rate.
Pinned native gravity setup (2095A8/29E9AC) and actual 2A2870 conversion establish
game Y-up, physics Z-up, and physics gravity (0, 0, -17). The free-limb fixture
uses that gravity; the horizontal loaded fixture models vertical support. The loaded
boundary shares native force increments over extra mass; it is a stress model,
not an execution of the native joint/contact solver. Ray geometry and
collision/joint solving remain external boundaries. Test scenes must check
arm/leg grabbing, release, pulling around obstacles, original Interact actions,
co-op ownership and reviving a grabbed partner at 30/60/high FPS.

The first candidate is private and uses the same runtime and manifests for
Windows and Linux/Proton. Its receipts live in `build/ragdoll-drag-20261105`.
Offline success does not establish live dragging feel or Proton compatibility.

The November 6 input-isolation candidate was confirmed working for controller
deadzone isolation, but the user could not grab bodies. Read-only snapshots in
`build/ragdoll-drag-followup-20261106` establish that the drag entry and input
gates ran, and a native ray reached a valid dead NPC within grab range. The
snapshot was paused and did not capture a held attempt or resolve the target's
overflow-pool limb index. Native ray fields, collision layers, center-of-mass
coordinates and wake/force arguments were checked; none justify a change yet.
The consumed-miss bug was corrected independently. The subsequent failed test
exposed a separate input error: dragging queried physical X (`0x18`, Cycle
Firearm) instead of A (`0x16`, Interact). The first diagnostic also inspected the
X byte (`pad+0x28`), so its absence of held attempts does not mean the user did
not hold F. The corrected query and private diagnostic use A (`pad+0x10`).
The fixture now executes the pinned native query and physical-button resolver
instead of returning a synthetic held value for whichever action was requested.
No ray, collision or force change was justified by this input failure.

The user subsequently confirmed that grabbing and dragging work. The incoming
November 6 evidence at `build/body-interaction-polish-20261106/incoming-evidence.json`
records three successful acquisitions and the tested runtime hash. That accepted
build still felt elastic. The current bounded-velocity pull and 18 cm lift address
that feedback without changing the successful input, latch, ray, identity,
co-op ownership or release paths. Their final live feel still needs confirmation;
the prior acceptance is not evidence for this new force tuning. Proton remains
untested, using the same maintained runtime as Windows.

The first moving-target fixture exposed residual rebound at 30 FPS under a
16x coupled load after stopping from 4 m/s: position was only 6 mm from target,
but speed was still 0.152 m/s after four stationary seconds. Reducing the
position gain from 20 to 10/second and increasing velocity correction to 1.4
addresses the cause; the moving-target position and velocity limits are
unchanged. Free-limb checks allow 0.31 seconds to reach 95% of a half-metre step
and at least 13 cm of actual lift under native gravity, while retaining
monotonic approach, force limits and no position writes. Evidence is retained
under `build/body-interaction-polish-20261106`; actual native fixtures and live
joint/contact behavior must be reported separately from scalar modeling.
The corrected native fixture passed, including the unchanged moving-target
endpoint limits, original held-input query, gravity conversion, wake and force
integration. The user then confirmed that the revised dragging response works
correctly in Windows runtime
`2cfc12b49902f174f19626312adf0ec6ca088ed3ee4154bd70f5e08e979c318a`.
This confirms that test's feel, not exhaustive contact or Proton coverage.

The next requested change admits head and chest hits with the same grab gesture,
force response and release rules. The original `ragdoll_havoc3.xml` asset
(`79d6ae7cc5750c322e5e0b971869212da6d86ef9b4d064e52adb9acab6178946`)
identifies physical body 4 as `Bip01 Spine2` (chest) and 8 as `Bip01 Head`.
Pelvis (0) and lower spine (1) remain excluded. The fixture checks all twelve
target indices, both co-op owners and single-player, the selected body's native
force, hold/drop, and rejection of living actors. This added target coverage
passed its compiled fixture in `build/body-shield-native-20261106/head-chest-checks.json`.
Live acceptance of the new head/chest targets remains pending.
