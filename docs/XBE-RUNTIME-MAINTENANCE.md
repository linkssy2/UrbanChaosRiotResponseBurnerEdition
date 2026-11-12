# Burner Edition: current runtime

## November 12: scope, blood pools, hotel audio and launcher cover

The user accepted scope artwork remaining visible with No HUD, restored blood
pools with ledge clipping, and a 300 ms advance of the TV electrocution sound in
Frank's Auto Parts. They also verified that knocked-out enemies do not create
blood pools. These are Windows user checks; Proton gameplay remains unverified.

`prepare_xbe_tv_audio.py` applies the accepted bank timestamp change after the
original AMPC parser returns at 00250451. The parser's GUID callback 0024AD10
writes the bank GUID at owner+34; track and reference chunks are at owner+98
and owner+90. The hook checks mapped extents, bank GUID, chunk headers, the TV
sample GUID and original command before changing 3662 to 3362 ms. Other banks,
dialogue and already-adjusted data are unchanged. The compiled fixture uses
the original bank and checks the exact changed bytes and rejected inputs.
This is a scene-specific correction, not a global audio latency adjustment.

Do not ship the private hotel restart archive or test saves. Main packages use
the original disc manifest and apply audio timing only in memory. Original
restart IDs remain RestartPoint_0=0 and RestartPoint_4=3. The user's approved
cover is `launcher/burner/cover.png`, with the gold/red Burner Edition lettering
over an orange brushstroke. Paired build/package evidence belongs in
`build/main-fixes-20261112`; previously published archives remain unchanged.

## Weapon selector acceptance and 0.17 preparation (November 8)

The user confirmed forward/reverse selector visuals on the normal candidate
`e70d7ea19d89a4e9ce1a6ff4826d47fe554f6190eca57bf546bfb097db98e321`, then accepted
cycling with ten firearm variants in private all-weapons runtime
`224fc9fd12433c55b83b216558001f9fb69fa92a723a1ce76da3cd2227c67311`.
The latter only adds a session-only menu toggle, native resource preloads and
native inventory grants; the maintained selector code is identical. This is
user visual acceptance of that longer inventory, not a full campaign or Proton
verification. The temporary cheat and copied test saves must not ship. Normal
Mk. progression is unchanged. Preserve private recipes/evidence under
`build/all-weapons-test-20261108`; release preparation lives in `build/release-0.17`.

## Separate frame ghosting control (November 8)

Graphics now separates Gameplay blur (scene-depth and co-op downed softening)
from Frame ghosting (previous-frame feedback). Both are saved shared settings
for single-player and both co-op views, including shared cutscene cameras. Apply
publishes immediately; neither option rebuilds display resources. Turn both Off
to suppress both screen effects. Keep glow, authored particle streaks, heat haze,
camera motion and prerendered movies separate from these controls.

New installs retain the original default-On behavior. When loading an older
settings file without `frame_ghosting`, inherit its `gameplay_blur` value once;
an explicit new ghosting value wins. This preserves the previous all-Off choice.
The final composite `000A2C70` uses FrameGhosting for its neutral local history
weight; `00097B50` and the co-op downed pass continue using GameplayBlur. Keep
effect aging and history rotation running while either presentation is disabled.

The report in `build/blur-report-20261108` confirms original MotionBlur events in
the matching train-crash animation. The user subsequently confirmed the reporter
had Gameplay blur Off; treat that as a reported failure, not proof the separate
toggle fixes the recorded effect. Exact running version and live reproduction
still require confirmation.
Implementation, paired candidate receipts and verification limits belong in
`build/frame-ghosting-20261108`. Source-backed fixtures and GPU sampling checks
remain distinct from live acceptance in those exact scenes and on Proton.

## Weapon selector direction (November 8)

The user's three-firearm recording confirms a separate presentation defect:
shotgun-to-pistol changes the ammunition display correctly, but the carousel
passes through Assault Rifle Mk.3. The original HUD updater `00077230` only
advances its fractional selection index forward. Reversing inventory selection
alone does not reverse that animation.

Keep the correction in `prepare_xbe_firearm_cycle.py` and
`xbe_firearm_cycle.h`. Remember the accepted Previous Firearm selection at
`00042230`, scoped to its inventory owner and target; use it only for that
owner's firearm carousel. Preserve the original animation speed, fade, renderer,
controller forward cycle, other categories and inventory eligibility. Direction
must survive key release and be replaced by subsequent forward/direct selection.
The retail-function fixture covers wraparound, rapid changes and co-op ownership.
Private evidence and paired candidate verification belong in
`build/weapon-selector-direction-20261108`; do not equate fixtures with live
visual acceptance or Proton verification.

## Private firearm cycling and optional mixed input (November 7)

Issue #27 reproduced offline with the production host input adapter and original
retail firearm consumer: a connection-discovery poll could clear the direction
after the guest cached a Previous press. With three eligible firearms, the
resulting forward step looks like two backward steps. Direction is now sampled
per thread and published with the guest input snapshot before wheel advancement.
The old implementation fails the thread-interleaving regression; the corrected
one passes 20 combined host/retail scenarios, including rebound keys. Evidence
is in `build/issue27-firearm-audit` and `build/issue27-wheel-fix`.

Issue #6 adds a default-Off Controller + mouse setting for single-player. Keep
menu ownership and co-op physical-port routing unchanged. Mixed gameplay retains
the native stick look and applies raw mouse once afterward; custom deadzones
apply only to physical-controller-owned axes. The separate reload edge excludes
keyboard Interact, and spin/conversation consumers accept either appropriate
source. Do not force keyboard mode globally. Controls, menu, host input, spin and
conversation fixtures cover defaults, saved state, device handoff and co-op
isolation; private evidence is in `build/issue6-mixed-input`. Gameplay feel and
live Windows/Proton acceptance remain separate from these offline checks.

Graphics and Audio descriptions now reserve space above the frame's bottom
border. The Graphics scrolling area and separator move with the help text so
the last setting cannot overlap it. The existing menu fixture checks actual
font/shadow bounds for Graphics, Audio and Controls, including English/Spanish,
original/HD fonts and 4:3 through 32:9 layouts. Evidence belongs in
`build/settings-help-layout-20261107`.

## Private gameplay blur option (November 7)

Graphics > Gameplay blur is a default-On, saved shared setting for single-player
and both co-op views. Apply publishes it immediately without rebuilding display
resources. Keep draft/Discard and failed-save rollback consistent with the other
graphics settings, including English and Spanish menu text.

The scene-depth blur consumer `00097B50` takes its original no-effect epilogue
when Off. Pulse creation, aging, expiry and co-op descriptor restoration remain
intact, so toggling On cannot revive an expired effect. The separate co-op downed
scene blur observes the same setting; the red warning, timer and revive rules
are unchanged. Do not suppress the generic Gaussian filter or authored blurred
particles: glow and other rendering use those paths too.

Before the separate Frame ghosting control above, the MotionBlur manager also
followed Gameplay blur. It publishes through `0005F890` to graphics `+28`
(`000898C0`, descriptor `004561D4`, strength setter `0009D480`). Its consumer is
`000A2C70`: override only the local strength after `000A30A9` with the original
neutral zero when Off. Retain the current-scene composite and original effect
lifecycle; skipping that entire function would remove more than motion blur.
The original strength getter and x87 pop still run, while the zero local prevents
previous-frame acquisition and supplies zero to the feedback shader constant.
Swap continues rotating frame history while Off, so enabling again reads recent
history rather than a frozen image. The fixture executes the pinned publication,
history-admission and shader-weight instructions, and checks default-On parity.

Source-backed scene-consumer, downed-state, settings persistence and menu fixtures
cover the option. Paired private build receipts belong in
`build/gameplay-blur-20261107`. The Windows tester confirmed that turning the
option Off removes the blur. Separate downed/co-op scene coverage and Proton
execution remain unverified by that feedback.

## Private environmental audio and healing correction (November 7)

The active xberecomp mixer previously acknowledged I3DL2 listener and effect-data
requests without processing the title's environmental audio. The maintained
adapter restores the game's bin-10 environmental send and bins 6-9 directional
processing using the pinned xberecomp DSP processors. Apply authored channel
headroom once, preserving separate voice headroom. Retail startup explicitly
sets all 32 channel headrooms to zero; retain that call instead of assuming the
SDK default of one. Keep deferred settings, effect
state bounds, unaffected direct-speaker routing, MIXIN gain, voice cursors and
stereo/surround output contracts under test. The game also submits per-voice
low-pass packets through buffer SetFilter and the stream entry mislabeled
SetEG; apply that filter once
before the dry/wet routing split. The inspected filter owners are PCM/stream
children, not the spatial MIXIN buses; do not invent shared-bus filters from
the adjacent bus constructor alone.

The original image includes Xbox Live communicator capture modules. These are
not missing environmental effects and no microphone or network voice input is
fabricated. Original executable/image identities, all-archive reverb census,
processor classification and waveform limitations belong in
`build/subway-reverb-20261107`. A compiled DSP adapter or offline impulse tail
does not establish audible Xbox parity or live Proton acceptance. The original
game requests FullHRTF; the existing PC spatial mixer remains an approximation,
and the host 7.1 side pair is an extension of the original front/rear processing.
Keep those limitations explicit rather than claiming complete DSP equivalence.

Stored self-healing now reserves a pack while the original injection is queued
or playing, then spends it at the original Heal animation event. An earlier
cutscene cancellation retains the pack and blocks a stale event. Preserve the
original self-animation, sound, pulse and gradual refill; partner healing and
reviving retain their existing successful-contact debit. The original queue,
state handler and post-contact refill are exercised in the health fixture,
including both co-op owners. Evidence belongs in
`build/health-cutscene-20261107`.

The combined private Windows and Linux/Proton candidate and shared source/runtime
receipts belong in `build/graffiti-audio-healing-20261107`. Keep live audio,
cutscene interruption and Linux acceptance separate from automated checks.

## Private View Controls replacement (November 7)

Controller Info now presents the same framed, scrollable native menu as the
other PC screens. It reads current action bindings for keyboard/mouse or either
controller profile; a co-op pause locks the reference to the player who paused.
Mouse wheel, draggable scrollbar, clickable rows and footer, right-click,
Escape and controller Back are supported. Binding changes remain in Controls.

The qualified original diagram snapshot owns presentation only after native
acceptance; Back still returns through the original main/pause callback. Keep
the original renderer fallback, held-input gate and transition revocation.
The frontend and ImGui fixtures cover these contracts and current bindings;
record live acceptance separately in `build/view-controls-20261107`.

The live Windows check covered entry, current bindings, scrolling and right-click
Back. Scroll selection now stays within visible rows, and the shared menu atlas
includes Up/Down keycaps used by View Controls and Objectives. Keep physical
controller/co-op pause acceptance and Proton testing separate from the fixtures.

## Graffiti transparency investigation (November 7)

The subsequent user request explicitly changes the authored appearance: retain
the lettering and dark outline, but make both translucent like the raw HD source.
The expanded census covers 25 distinct graffiti/tag identities and 89 material
references. The runtime treatment halves final opacity after the
original alpha test, preserving RGB, cutout coverage and authored over/additive/
reverse-subtract blending. Exact source identity also covers existing HD
replacements without modifying archives or requiring a new texture pack.
Keep the census and candidate verification in `build/graffiti-alpha-20261107`;
this is an appearance enhancement, not evidence of alpha stripping in the port.
Record exact-identity and alpha-test verification separately from live appearance
acceptance. Shader-header preparation accepts fresh and already-patched copies,
preserves the co-op blur constants, and rejects unexpected flag declarations.

The reported pink graffiti matches original `map_02_graff03.tga` (64x64 BC3).
Its dark edging is authored in both the Xbox and matching original PS2 image;
the published HD pack already maps it. Thirty-two offscreen tests using the
production-generated compact material shader preserve original texture alpha
and blend transparent/fractional pixels correctly. No texture or renderer patch
was justified. Exact reporter level, HD settings and live draw state remain
unknown; retain evidence in `build/decal-texture-20261107`, not release assets.

## Private menu edge correction (November 7)

The full-viewport menu window now disables ImGui's window border while beginning
the window. Its default one-pixel border inset the content clip on all four
edges, exposing the moving original menu behind otherwise full-size artwork.
The override is restored immediately; nested content clips, original controller
screens and the authored animated background retain their existing behavior.

The production-derived menu fixture checks clipped opaque coverage at all four
corners across menu transitions, screen sizes and aspect ratios. Keep the old
inset-clip negative witness and private evidence in `build/menu-edge-20261107`.
Record live Windows menu inspection separately from untested Proton behavior;
this rendering correction is not an additional FPS claim.

## Private rebinding correction (November 7, issue 26)

Keyboard/mouse binding capture now retains fresh window-message presses, including
short taps completed between guest pad polls. The prior polling-only path could
miss those taps while ordinary menu toggles still worked. Capture starts armed
only when existing inputs are released; release events and the capture UI service
neutral arming without waiting for another guest input poll. Repeats, focus loss,
Escape cancellation, modifier sides, mouse buttons and co-op player ownership keep
their existing rules. Controller capture retains its physical polling path.

The production-derived input fixture includes a negative witness for the old
capture and covers complete press/release pairs between polls. Private evidence
and paired packages belong in `build/rebind-input-20261107`. The normal build also
retains the preceding CPU/rendering improvements; this input correction is not
claimed as an FPS gain. Record Windows menu acceptance separately from untested
Proton input and do not infer the reporter's exact hardware cause from one test.

## Private helper provenance follow-up (November 7)

The executable input graph and recorded compiler dependencies were audited under
`build/helper-provenance-20261107`. The game links the pinned XboxXBERecomp runtime
with current title adapters, not the retired SpoonZ renderer. Offline migration
fixtures remain reference evidence; deleting unlinked tests cannot improve FPS.
Older asset readers, the cursor and input adapters still support current features.

The default-off typed pixel-lighting adapter is no longer injected into the
production renderer. Default HLSL/DXBC, shader keys and cache behavior remain
identical. Keep the preparer's SUM operand and infinity corrections and independent
offline reference fixtures. This retirement is cleanup, not a measured FPS gain.

Co-op interaction calls now capture their temporary stack only when dispatching
guest code or writing event arguments. Lock captures before its pre-call payload;
repeated calls retain the first snapshot and exception unwinding restores it.
The differential fixture includes nested calls and nonzero argument sentinels.
Runtime guest-memory, resource-lifetime and synchronization checks remain active.
Correctness proofs run in separate tests, not in the gameplay frame loop.

Sampled render targets reuse the current draw's target node without repeating
the target-map lookup or same-frame freshness write. Current header dimensions
and translated data identity are still checked. Extension callbacks retain the
original path; texture binding cannot retire the cached node in the ordinary
path. Preserve the final target rebind, which restores attachment states after
copies and resolves. The production-derived fixture compares resource-operation
order and includes a failing unchecked-pointer witness for mutable headers.

The original maximum-health query writes 332 bytes below the live caller's stack
pointer, exceeding its former 256-byte snapshot. A query-local 512-byte guard and
snapshot now cover that footprint; self-healing uses the same protected query.
The original five-function chain, upgrade/profile cases and outer sentinels are
executed in the health fixture. Other scratch users are unchanged and are not
claimed transitively depth-proven. This correction is not an FPS optimization.

The preceding candidate's matched Windows Canals A/B/B/A runs measured 67.86 to
72.46 FPS (6.78%) at unchanged visual settings. That gain belongs to the earlier
state/bounds/readback changes in `build/performance-deep-20261107`, not this
follow-up. New helper timings do not establish another gameplay FPS gain.
Maintain one resulting runtime and asset manifest for Windows and Linux/Proton;
separate live acceptance is required, especially for Steam Deck.

## Private CPU/GPU helper review (November 7)

The shared Windows/Proton runtime now prepares three qualified candidates:
audited simple-state dirty blocks, the current-ABI lighting bounds leaf, and one
reusable synchronous downsample surface. Render-state replay still owns an
immutable queued copy; guest resource headers remain checked on every draw.
Untracked state and unsupported bounds inputs keep the existing path. The
downsample change preserves filtering, alpha, barriers and blocking readback
waits; asynchronous readbacks retain their independent storage.

Production-derived replay, generated-code equivalence and hardware/WARP pixel
checks run through `scripts/build_xbe_game_trial.py`. Evidence and the frozen
private source manifest belong in `build/performance-deep-20261107`. Helper
microbenchmarks do not establish a gameplay FPS gain. Final Canals comparisons
must use matched settings, uncapped rendering, timed restart phases and both
co-op views; Windows results do not establish Proton or Steam Deck performance.
No effects, geometry, resolution, lighting or texture quality is reduced.

## Private menu input handoff and Steam Deck review (November 6)

The Steam Deck v0.14 support log shows a 1920 x 1080 window on the built-in
screen, with sparse gameplay samples of 33.4-58 FPS. It does not identify CPU/GPU
costs. See `docs/STEAM-DECK-INPUT-2026-11-06.md` for the resolution comparison and
limits. Do not claim that the menu fix improves Steam Deck frame rates.

Ordinary menus now follow fresh device activity: unchanged held controller
inputs and stick noise do not reclaim ownership from the mouse/keyboard.
Controller ownership suppresses stale pointer hover, queued mouse/key events and
native pointer actions; mouse/keyboard ownership suppresses queued controller
navigation. The co-op character selector retains simultaneous independent input,
and co-op pause ownership remains unchanged. The main menu highlights only its
focused row.

Private source backups, automated-check and paired-package receipts belong in
`build/deck-input-20261106`. The existing private knockout-drag change is retained.
No game or published release is replaced without the corresponding staging step;
Windows and Proton live device switching and Steam Deck performance require
separate confirmation. Normal logging remains opt-in.

## Private knockout dragging follow-up (November 6)

Extend the shared drag eligibility to knocked-out NPCs with positive health,
using the original DF380 restraint flag and the exact currently owned physics
wrapper/data/pool handle. Dead NPCs and downed co-op partners retain their
existing rules. No input, health, AI, arrest, recovery, force or retention policy
changes are needed. Original player input and arrest behaviors were traced;
there is no evidence for delaying or consuming ordinary Interact presses.
The original knockout event is pinned and executed in the drag fixture, with
actor-state preservation and invalidation checks. See `docs/RAGDOLL-DRAG.md`.
Private Windows/Proton candidate evidence belongs in
`build/knockout-drag-20261106`; published 0.14 remains unchanged. Native physics
feel and live arrest/recovery need a gameplay pass; Proton remains untested.

## Release 0.14 preparation (November 6)

The post-0.13 changes are prepared together as 0.14: 16:10 display support,
controller-only deadzones and reorganized settings, latched ragdoll dragging,
and the optional native Burner body shield including static first-mission table
pickups. Details and scoped acceptance are in `DISPLAY-AND-CONTROLLER-SETTINGS.md`,
`RAGDOLL-DRAG.md` and `CORPSE-SHIELD.md`.

Paired packages and checks belong in `build/release-0.14`; source closure and
release notes belong in `publication/0.14`. Published 0.13 remains unchanged.
The existing 0.09 HD download is compatible and is not repackaged. Release notes
are the release body, not a separate asset. Logging remains opt-in; developer
probes, personal state and original disc assets are excluded from the packages.
Static pickup placement/collection and live Proton gameplay remain unverified.
Preparation does not authorize publication.

## Release 0.13 preparation

The November 5 post-0.12 changes below are being prepared together as 0.13.
Versioned Windows and Linux/Proton archives, source closure, update-preservation
checks and their receipts belong in `build/release-0.13` and `publication/0.13`.
The existing 0.09 HD game texture download is retained unchanged. Release notes
are the release body, not a separate downloadable asset. Preparation does not
authorize publication or establish new live Windows/Proton gameplay acceptance.

## Co-op hazard ownership and waiting messages (November 5, 0.13)

The user's Canals clip shows P1 standing in ambulance fire without losing
health, while P2 loses health in the same fire. P1 subsequently takes other
damage, so the evidence does not support general invulnerability. Retail
`1726C4 -> 128F0 -> 4A780` increments the player count but overwrites the single
ID at manager `+834`; `4ACD0` ignores its requested index. With P2 registered
last, the original FXFire, FXElectricity and FireDamageSphere loops therefore
visit P2 twice and miss P1. Repeated visits also affect the native burn cooldown.

The shared correction replaces only the returned ID at the three verified
hazard lookup callers, for a validated active co-op pair. Original damage,
distance/obstruction checks, immunity and cooldown handling remain in place.
Unrelated singleton lookups and single-player retain their original behavior.
Collision-driven FireJet uses a different actor-targeted route and is not
rewritten by this fix.

The clip also shows a persistent P1 "Waiting for partner" message through
movement and P2's down/revive cycle. Conversation records kept an old wait when
retail busy/condition branches bypassed admission, after revival during speech,
or after the recorded behavior/NPC became invalid. Waiting and skip publication
are now renewed by each update; invalid records are retired with identity-safe
cleanup of their owned control locks. Genuine script-completion waits still
require both living players within the authored NPC range. No script is skipped
and no message timeout is used to conceal a pending transition.

Private evidence and regression receipts are in `build/coop-wait-fire-20261105`.
The three waiting histories fail with the preceding producer and pass with the
correction while executing the original guest conversation machine. These
offline checks do not establish final live Canals or Proton acceptance. The user
subsequently confirmed that the waiting message clears and fire damages both
players in the staged Windows runtime
`c17f0760fae5c4bac885941d5de6a9c664f91018a7582bb4c4aa8df9bc2f7d0b`.
Electrical hazards and live Proton remain covered only by offline checks.

## Co-op aiming blur isolation (November 5, 0.13)

The normal aiming effect previously merged both players' zoom pulses into one
shared blur descriptor, then copied and composited the whole scene. The shared
runtime now records each zoom pulse's owner while retaining the original pulse
timing, allocation, deletion and global publication. Ordinary split-screen
passes select the current camera's pulses plus the shared effects; single-player
and full-screen cinematic passes retain the original aggregate.

The original depth blur crops its source and final quad to the owning view.
The first candidate's live test failed: both cameras entered the effect with
a full-screen device viewport. Its assumed half-height viewport caused both
source-crop paths to reject, despite correct pulse ownership. Player 2 renders
last, explaining why its full-screen effect visibly changed Player 1's view.
The correction derives scene dimensions from the validated original downsample
quad, whose extents come from the backbuffer description. It accepts either the
full scene or the matching owned viewport, and keeps incoming viewport restoration
separate from the crop. The recorded full-screen case is now a regression for
both owners; unrelated effects and malformed geometry retain their original path.
The original scratch textures and Gaussian filter remain; vertical filter taps
compensate for placing a half-height scene in the full scratch. A narrowly
qualified source-copy shader clamps samples to the owning view's actual GPU
pixel centres, including when a render-scale change is pending. Its bounds are
captured with the queued draw. Both ordinary and upscaled box-filter sampling
clamp each tap, preserving the original weights and colour scaling. Other shader source and cache identities remain
unchanged. This does not replace the separate downed-player blur or change aim,
reticles, FOV, game assets or logging settings.

Trace, prior runtime and verification receipts belong in
`build/coop-aim-blur-20261105`. Native pulse/queue and geometry fixtures, GPU pixel
checks, production integration and live acceptance are separate gates. The
Windows and Proton candidates must use the same corrected runtime. The user
confirmed that zoom blur no longer crosses views in the normal Windows candidate
`fd05224da7eb8fb2b7d4b02a23a246024d53f8ea255eed2e9651ad4bb40761cf`.
Live Proton gameplay remains untested.

The native pulse/queue and geometry regressions passed, including original
lifecycle/CPU/FP parity, owner changes, exception cleanup and cinematic
transitions. The production shader/constants passed 54 GPU pixel cases on each
of Windows hardware and WARP, with both owners, five render scales, three
aspect ratios, odd host heights and negative leakage witnesses. These GPU checks
use D3D11 to execute the shared generated shader; they do not establish live
D3D12 or Proton gameplay acceptance. Full release-build and paired-package
receipts are recorded separately under `build/release-0.13`. The failed candidate's
bounded owner/render trace and follow-up checks are preserved in
`build/coop-aim-blur-followup-20261105`; the subsequent user confirmation above
applies to this corrected candidate, not the failed one.
The follow-up passed 38 original-quad and invalid-state cases, native pulse/queue
tests, delayed draw-state replay, and 54 GPU sampling cases each on hardware and
WARP. These include the recorded full-screen viewport for both owners.

## Canals reverse-side road railing (November 5, 0.13)

The reported opening camera crosses behind one-sided strips in the original
Canals road-edge mesh. A census of all 3,403 authored geometry batches found
22 coplanar strip triangles, all facing the same direction, in two batches.
Their texture and vertex alpha are opaque. No opposite strip faces were found.
This establishes an exposed authored-geometry limitation, not a proven change
from the original console rendering.

The first repair admitted only the ordinary texture pass. Its hardware/WARP
coverage fixture passed, but the user then observed transparent-looking metal.
That fixture used an opaque test shader without depth and missed the original
multipass contract: `99240 -> 977A0 -> BB870 -> 8FF30` supplies ambient RGB and
depth, then `96BE0` multiplies the texture into existing RGB with `DST_COLOR/ZERO`
and no depth writes. Culling the first pass therefore textures scenery behind
the strip. This is not texture alpha: the installed HD replacement's 43,695
valid texels are opaque across all mip levels; zero-alpha block padding is
outside the image.

The correction admits both original shader pairs for the same two exact
position/index streams. The added prepass requires its exact position-only
declaration, depth writes, RGB writes and disabled blending. Shadow/depth-only
uses remain excluded; alpha and all blend/depth state remain unchanged. The
regression must exercise the original shaders and multipass composition, with
background independence and depth occlusion as well as coverage. Private
evidence is in `build/rail-opacity-20261105`. Fixture bytes are derived locally
from the user's original game files and are not distributed. Live corrected
Canals cutscene and Proton acceptance remain separate requirements.

The revised fixture passed on hardware and WARP: two of 3,403 batches per
original shader pair, 3,248 negative guards, and 21,920 formerly background-
dependent pixels. Corrected pixels no longer depend on background color;
farther geometry is occluded and nearer geometry stays visible. Front-side,
unrelated geometry and framebuffer alpha are unchanged. These are D3D11 pixel
tests with controlled geometry projection, not live D3D12 cutscene acceptance.

## Helper-tip alignment (November 5, 0.13)

The native helper-text queue discards the widened HUD descriptor, leaving its
fixed 4:3 coordinates behind while the orders icons follow the view center.
The shared help draw now retains that center-relative horizontal offset in
single-player and each co-op view. Choose Heal, Display Orders, weapon hints
and other tips using this queue move together with their mouse/key/controller
glyphs. Existing co-op vertical placement, text wrapping, fades and input
ownership remain intact; projected target brackets and menu text use separate
paths. No new setting, asset replacement or logging is introduced.

The native help-queue fixture samples short and long lines across five aspect
ratios, both co-op draw orders and single-player/shared-cinematic guards.
Evidence and the prior implementation are in `build/heal-prompt-layout-20261105`.
Its font boundary uses measured-size test boxes; final localized appearance and
live Proton remain separate verification requirements.

## Maintained-code audit (November 5, 0.13)

See `docs/CODE-AUDIT-0.13.md` for scope, fixes, cleanup and verification limits.
The audit separates co-op health-bar history, completes Controls Defaults for
Walk/HUD bindings, and reuses checked atomic crash reporting. Empty geometry
wrappers and the rejected brightness experiment are removed; generated-tree
cleanup, upload alignment, failure handling and opt-in support logs remain.
Original experiment sources and detailed coverage are preserved privately in
`build/code-audit-20261105`. Rebuild and refresh both unpublished packages and
their source archive after these changes; old preparation hashes are superseded.

## Movement inertia and launcher layout (November 5, post-0.12)

Controls adds per-player **Movement inertia**, default On. Off uses the resolved
directional targets at the native BF2C8 seam instead of their smoothed outputs.
Starting, stopping and reversing input respond immediately; maximum speed, Walk,
crouch/equipment restrictions and downstream movement physics remain original.
Only valid, living, owned main-player actors in ordinary movement are eligible.
Pause, special movement, ladders and invalid state retain the original path.
No sprint, deadzone changes, new assets or runtime logging were added.

The native-context fixture passed 48 default/reference comparisons, then checked
immediate directional changes, analog input, Walk/crouch, co-op ownership,
frame-rate changes, impulses and guards. Controls, co-op controls/input, graphics
input, menu, language and aim-mode regressions passed. Evidence and source hashes
are under `build/movement-inertia-20261105`.

The existing WinForms launcher now groups Manage into Settings, Game Files,
Add-ons, Updates and Support. Explicit table rows/columns keep navigation and
action buttons consistently sized at minimum/default sizes and scaled layouts.
A separate flexible spacer prevents Support from consuming the remaining height.
The artwork frame fits the full cover at its original proportions, without
cropping. Play and Manage have matching size. Backend setup/update behavior and
saved language/startup choices are retained.

The launcher fixture passed grouped navigation, scaled layouts, keyboard focus,
worker failure/cancellation recovery, saved choices and real Windows argument
round trips. All seven final offscreen page renders were visually reviewed.
These establish layout checks, not final live desktop or Proton acceptance;
desktop control was left with the user. Live movement feel remains unverified.

The Windows and Linux/Proton test folders share 333 identical payload files,
runtime SHA-256 `f1ca7faab58020fa4334d3957637b6b1a547cc2e3c2cf03806caf47367af955b`
and launcher SHA-256 `49874a91b3a75c0bf9a22bbf7d7f47e312fc44a176f19d4a9965c0ab57a43501`.
Package/setup and Linux-wrapper checks passed; three Linux-only checks were
skipped on Windows. Live Proton remains unverified. The private imported play
folder received the same payload while preserving settings and Skip Startup.
It is not a distributable package. Nothing was published or version-bumped.

## Startup option and health-upgrade HUD fade (November 5, post-0.12)

Launcher Manage adds **Skip Startup**, default Off. Its validated local saved
choice alone enables `UCRR_SKIP_STARTUP=1` in the game process; inherited probe
flags remain cleared. Windows Setup and Linux/Proton Setup share the control;
the Linux shell alternative is `./burner.sh startup on` or `off`. Toggling it
preserves an unsaved language selection. Run/sprint and dead zones are unchanged.

When enabled, only the four exact unopened startup movie/successor pairs are
skipped. Their original factories still initialize the frontend and their
destructors retain normal object cleanup without closing an unopened global
decoder. The direct-profile refinement retains the original loading/fade gate,
then runs `271E80 -> 271C00` through its shared frontend-bank and controller
initialization. Only that armed startup call redirects at `271CCD` to the
original `2717B0` profile factory, before the Press Start tree is constructed.
The original output pair, controller association and profile/storage admission
remain intact. It does not select a profile. Outgoing skipped movies stay
unopened until their normal cleanup. Mission films, looping backgrounds, held
skip controls and later title visits retain their existing behavior. The native
update/destructor/factory fixture verifies these boundaries and input handoff.
Direct startup needs a fresh live check; the earlier user confirmation covered
the automatic Press Start acceptance, not an absent intermediate screen.

The running game and splash window classes already requested icon resource 1,
but the game executable contained no icon resource. The maintained build now
embeds the launcher's existing `burner.ico` at that ID. No window ownership,
renderer or input behavior changes. A read-only post-build check compares each
embedded image with the source icon and exercises the production Windows icon
lookup. The same executable ships for Proton; desktop/taskbar appearance on each
platform remains a separate live check. Private evidence for both refinements
is in `build/startup-direct-profile-20261105`.

The user's video also shows a dark health-upgrade divider alone before the HUD
appears. Both native marker loops used fixed opacity `0x55`, ignoring the HUD
descriptor's fade. The shared leaf now scales only this alpha by the descriptor
alpha; black color, geometry, upgrade count and draw routing remain unchanged.
The 168 original/fixed loop/resume cases traverse the original sprite queue,
covering tiers, descriptor fades, single-player and split-screen origins, CPU/FP
state and memory guards. Existing parent-HUD fade and health-pack checks passed.

Evidence is in `build/health-marker-reveal-20261105` and
`build/hud-startup-options-20261105`. Release-profile runtime, launcher UI,
package/setup, Linux wrapper, menu, settings, view layout and FMV checks passed.
The launcher fixture now runs under the normal WinForms message loop; its prior
ad-hoc-loop teardown failure is preserved with the failed check records.
The Windows/Proton candidates share the same runtime and payload manifests.
No release was published and no game or HD texture assets changed. The user
subsequently confirmed both startup skipping and the corrected HUD reveal work
in the staged Windows candidate. Launcher Manage was also observed with its
saved toggle visible. This is user-reported gameplay acceptance, not an agent
frame comparison. Live Proton remains unverified.

## Live audio selection clarification (November 5, post-0.12)

In-game Stereo, 5.1 and 7.1 changes already apply during playback. The Audio
footer now states this explicitly in English and Spanish instead of presenting
the separate Windows speaker-layout restart advice as a general requirement.
The mixer samples the saved selection once per 480-frame block; the endpoint's
fixed channel capacity and queued audio stay intact. Selecting a wider mode than
the device supports retains the existing fallback and Active output display.
No device recreation, voice reset or added runtime logging was introduced.
Changing the operating system's speaker configuration is a separate operation.

The production mixer regression now switches eight times with a looping source
and a three-packet stream at 2/6/8-channel capacities. Positions, ownership and
ordered packet completion requests match the unchanged reference mixer. The
production output-worker fixture verifies mode changes without source-voice
recreation or queued-buffer modification, including endpoint fallback/failures.
The actual menu fixture verifies immediate active-mode changes. These checks,
audio routing/configuration/loop tests, language/settings tests and 800 original
FMV draw cases passed. Physical surround audibility is not established by them.

Evidence is in `build/audio-live-switch-20261105`. Windows and Linux/Proton test
packages share 333 identical payload files and runtime SHA-256
`e8dea628957e1da4d52a7cb5db15dd4f72a5134bae77f87ad1658a3f40ac6e32`.
Setup and Linux-wrapper checks passed; three Linux-only checks were skipped on
Windows. Live Proton and physical surround remain unverified. The private test
folder preserves the user's settings, including `fmv_scale=2` (Fill stretch),
and the accepted corpse-retention and convulsion changes. Nothing was published.
The private launcher started the matching runtime with Fill stretch selected;
the Rocksteady intro was visibly rendering before handoff to the user. Launch
identity and settings are recorded in this candidate's `sessions` directory.

## Forever regional-cleanup correction (November 5, post-0.12)

The shared character, persistent-character and general deletion-region updates
now respect Forever for killed NPCs with a valid owned ragdoll. The all-level
asset census found 129 instances across 17 of 27 ENT files; all three classes
dispatch through the same two corrected routines. There are no level IDs,
coordinate exceptions or asset edits in this fix. This closes a verified bypass
of the timer/count policy. The user subsequently visited the same reported area
on Windows and confirmed that the corpses did not unload. This is user-reported
visual acceptance; no before/after actor capture establishes the original trigger.

Selection checks run after native containment, at 1C9CB7 and 1B3CE9; bodies
outside a volume do not incur the extra ownership search. Queue-drain guards at
1C9DB2 and 1B3D35 handle IDs queued before death or before Forever was selected.
Both generated general-region entries are covered. Protected IDs are removed
from the queue normally, while persistent scans do not re-enqueue them. Original
script update and region deactivation continue. Default and More retain their
existing regional behavior. There is no per-frame allocation or added logging.

Eligibility requires an original NPC class, native killed flag (not knockout
alone), finite nonpositive health, and its current physics-interface handle
resolving to an active retained ragdoll. Living/injured/knocked-out actors, props,
foreign actors and already explicitly removed actors are not protected. Direct
mission removals remain original. Checkpoint/level reset destroys actor lists
through the independent 208D90 teardown, so Forever does not retain the old world
across restarts. Evidence and boundary reviews are under
`build/corpse-retention-20261105/fix-review`.

The release-profile runtime and six regressions passed: original regional loops
and queue compaction, persistent scans, native/overflow bodies, all retention
modes, dead-after-KO, invalid ownership, CPU/FP state, explicit removal, pool
teardown, death convulsions, contacts, recovery, co-op death/revive and health packs.
The level/source/build receipts are in `build/corpse-retention-fix-20261105`.
Windows and Linux/Proton test packages share 333 identical payload files and
runtime SHA-256 `76573c347277ce1fc6f11b16750ace62be3c0f10db23bcf8d6fb0f4d2127f49a`.
Setup and wrapper checks passed; three Linux-only checks were skipped on Windows.
The private imported play folder is updated, but is not a distributable package.
Accepted convulsions and FMV Fill stretch are preserved. The runtime was launched
for the user's Windows test, with video observed before handoff. User acceptance
is recorded under `sessions/user-20261105-134140`; no release was published.
Other levels and live Proton corpse retention remain unverified.
Retaining more corpses can increase memory/physics cost, as with the existing
Forever setting. No new HD pack is required.

## FMV Fill stretch candidate (November 5, post-0.12)

Graphics adds **Fill stretch** as saved `fmv_scale=2`. It uses the exact
all-frame black-row bounds from the existing 42-movie audit and maps the remaining
picture to the full display. It stretches proportions rather than cutting scene
content. Existing Fill (0, still default) retains its proportional fit and four-row
margin; Original aspect ratio (1) retains the complete encoded frame. News and
unknown films stretch their complete frame; unverified custom-film borders are
not automatically guessed. Menu movies and the alternate 448-line draw are unchanged.
Windows and Linux/Proton use the same runtime change. Verification is recorded in
`build/fmv-stretch-20261105`; no game launch or live Proton acceptance is implied.
The release-profile build and 800 original-guest FMV draw cases passed, including
4:3 through 32:9, source coordinates, existing modes and background movies.
Settings persistence, actual menu navigation, Spanish, skip input and health-pack
regressions passed. Clean Windows and Linux/Proton test packages contain 333
identical shared files; their setup and wrapper checks passed (three Linux-only
checks skipped on Windows). The imported private play folder has the same new
runtime. Accepted revision 8 convulsions are retained. Nothing was launched or
published, and no new movie assets or HD texture pack are required.

## Death convulsion candidate (November 5, post-0.12)

The user visually accepted revision 4's enemy convulsions on Windows, then
requested stronger variation, short bursts with pauses and an On/Off switch.
Revision 5 added **Graphics > Death convulsions**, beside Ragdoll amount, default
On. The shared setting covers killed enemies and downed co-op partners. Apply
saves `death_convulsions`; Off cancels active reactions, and re-enabling admits
later deaths rather than restarting expired effects. The row uses the existing
save/discard/failed-save rollback behavior and includes Spanish text.

The user rejected revision 5's appearance as repeated leg/foot kicking.
Revision 6 uses all eleven authored joints: spine, hips, chest, knees, shoulders,
neck and elbows. Three 0.38-0.56-second bursts share a 7-9 Hz contraction rhythm
with small variations in joint phase/strength, separated by quiet intervals.
Timing and strength vary deterministically by actor identity without touching
gameplay RNG. The reaction retains its 5.4-second maximum and starts after 0.6
seconds. Knee/elbow travel is much smaller; torso/shoulder motion now participates
and neck motion is deliberately limited. Equal and opposite angular impulses
pass through the original wake and inertia routines; the native solver still
controls joint limits and contacts. The reflected ragdoll basis uses planeAxis
at data +0x20, not its pivot at +0x10; hinges retain axle +0x30.
There is no added linear impulse, per-frame allocation, logging or asset dependency.

The user described revision 6's whole-body motion as almost perfect and requested
slightly stronger bursts. Revision 7 raises spine, chest, hip and shoulder travel
by 15%, and their angular speed ceiling from 8 to 9 radians/second. Knees, elbows,
neck, burst timing and acceleration bounds retain revision 6's values. The guarded
build, existing 30-360 FPS fixtures, native contact/recovery checks, co-op revival
and settings/menu regressions passed. Paired clean Windows/Proton packages and
the private play folder were staged and launched for user testing.

After revision 7, the user requested another 30% increase and explicit head spasms.
Revision 8 multiplies travel, speed and acceleration ceilings by 1.3 relative to
revision 7. The neck's base travel increases from 0.045 to 0.10 radians and its
base speed ceiling from 2 to 5 radians/second, before the shared multiplier.
Burst cadence, gaps, native joint limits and paired-impulse conservation remain.
The fixture verifies head rotation in both directions with 19-20 reversals at
30-360 FPS for both enemy and downed-player admission; this is fixed-pose impulse
evidence, not live head-animation acceptance. The stronger isolated-hinge travel
budget is 1.3 times its previous bound; joint overshoot bounds are unchanged.
All existing regressions and paired setup/wrapper audits passed. The user then
visually accepted revision 8's convulsions and confirmed the On/Off toggle works
on Windows. Downed co-op appearance and Proton remain separately unverified.
There is no new asset pack or publication.

The initial revision 5 sharper-burst fixture exposed joint overshoot. The current
hinge controller retains a full step of braking distance before either joint
limit, bounded angular speed/acceleration and a smoothed burst envelope. Negative
results remain in `revision5/test-burst-initial-failure.log` and
`revision5/test-envelope-failure.log`; the bounds checks were not relaxed.

Admission follows native ragdoll activation at 2A6119, and updates run before
native pose writeback at 2A5E7C. Both generated entry/resume copies are patched
reversibly, with original-byte and generated-body witnesses. NPC transition
2AC0D0 gets its actor from the capsule proxy's virtual +0C: constructor 2BA430
stores the actor at +10, vtable 48F578 selects 290010, which returns that actor.
The original `enemy of player` query excludes allies, and positive-health or
knocked-out NPCs are not admitted. Co-op admission uses the existing downed-body
identity; revival, actor/model reuse and world changes cancel the effect.

The maintained build includes `test_death_spasm`, executing the original
angular-impulse, wake and boolean-property wrapper with explicit fixture
boundaries for property lookup and already-awake islands. It checks elapsed-time
behavior at 30/60/120/240/360 FPS, balanced angular momentum, untouched linear
velocity, CPU/FP preservation, eligibility, expiry, revival and reuse. Revision 6's
60 straight/folded, enemy/downed, actor-identity and frame-rate cases produce
18-26 isolated-hinge direction reversals, with peak travel of 0.168-0.439 radians
(revision 5 was 0.916-1.020). Ten further cases load all eleven original authored
constraints, transforms and anisotropic inertias and verify angular responses in
all twelve body sections, including torso and pelvis. Those poses are held fixed
to check impulse injection, not simulated world contacts. Separate checks cover
variation, quiet intervals, toggle cancellation, persistent settings and failed saves.
The actual ImGui fixture checks mouse/controller operation with original fonts,
Spanish and centered ultrawide layouts. Existing
force/contact/recovery, co-op death/revive, health-pack and
downed-camera regressions also passed. These checks do not establish the visible
contraction strength or combined live contact behavior.

Evidence is under `build/death-spasm-20261104`. The user saw no visible spasm in
three earlier candidates; those are negative visual results, not accepted fixes.
Revision 3's bounded read-only capture recorded 15 complete reactions and changing
joint angles, including elbows/knees resting at their upper limits. These samples
do not isolate the effect from falling/settling. The user clarified that they want
obvious repeated convulsions; revision 4 is the first visually accepted effect.
Revision 8's `verification.json` pins its normal executable and matching symbols;
`staging-check.json` records matching Windows and Linux/Proton test payloads,
shared manifests and separate setup/wrapper checks. Both use the same runtime;
Proton is not native Linux. The user accepted revision 6's whole-body motion on
Windows, subject to the requested small strength increase. Downed co-op appearance
and live Proton behavior have not been separately confirmed. Nothing has been published.
The imported private play folder contains copied user/game data and must never
be distributed as a release ZIP. The separate clean Windows-test/Linux-Proton-test
folders passed allowlist and original-payload audits. No new HD pack is needed.
The accepted runtime and matching symbols remain in `revision4/normal`, and
revision 5 remains in `revision5/normal` for comparison. Revision 6's shared
runtime SHA-256 is `7877c4d0f66d7fa52c63441b236db580d0458c78e0d6bc31783e8f9d70400dcd`.
That visually accepted whole-body candidate is preserved in `revision6/normal`.
Revision 7's shared runtime SHA-256 is
`fa9ef27b365df124a06eb18f0211af9fb1528fdaf47c4326ea7d2dc9c42e7d8a`.
It is preserved with its symbols in `revision7/normal`. Revision 8's shared runtime
SHA-256 is `4e1a7e9aee854a95e31dda457b1bf9bd4a47cc06fe839867b58acab3c1615558`.

Before the regional-cleanup correction above, the Forever-corpse report remained
unresolved. Native cleanup and expanded
pool regressions pass, but no instrumented deletion/release requests occurred in
the latest Hope Street capture. That does not establish corpse retention in the
reporter's container area. Mission-directed removal has not been disabled.
The November 5 offline audit confirms retention code matches published 0.12,
traces the NPC's separate missing-handle removal query through the expanded pool,
and reruns retention, contact and recovery fixtures successfully. Findings and
original-code trace are in `build/corpse-retention-20261105`. No game was launched
and no cause or fix for the reporter's disappearance is claimed.

The follow-up original-byte and authored-level audit confirmed a separate
level-driven bypass: CharacterDeletionRegion update 1C9BF0 (also used by the
persistent variant) can select retained registered corpses and sends removal
flag 1 at 1C9DB6 without consulting Forever. World update 208F80 subsequently
deactivates and destroys those actors. Eastside Dockyard (4-01), the likely but
unconfirmed screenshot mission, has three such regions with progression-script
activations; one covers authored ladder/container-area positions. Its separate
DeleteCharacters script is checkpoint-restart cleanup. The all-level census
found 129 general/character/persistent deletion regions in 17 of 27 ENT files.
Original/prepared Dockyard data match and all its script streams decode fully.
Evidence is under `build/corpse-retention-20261105/{caller-audit,level-audit}`.
This proves a route around the setting, not the reporter's exact actor/trigger.
Archived same-owner physics samples also show substantial descent while still
simulated, without identifying whether a surface was crossed. No game was
launched, production behavior changed or new package produced for this audit.

## 0.12 release review (November 4)

The review against published 0.11 found a mouse-coordinate regression in the
centered ultrawide menu. Setting arrows and sliders now subtract the drawn
canvas origin, as does the Controls scrollbar. The unused viewport member was
removed. The production-ImGui regression reproduces the wrong left-arrow
direction before the correction and checks slider endpoints/midpoint at 21:9
and 32:9 afterward. Existing callers share this correction.

The ultrawide paths use bounded arithmetic and existing layout scopes, without
new per-frame allocations, I/O or diagnostic output. Release preparation keeps
heavy diagnostics compiled out; session logging remains explicitly opt-in and
off by default, and essential error reporting is retained. This is a code and
release-overhead review, not a new performance benchmark or a claim that no
further optimization is possible. Paired 0.12 preparation, frozen input hashes,
repeatability, package allowlists and upgrade evidence are in `build/release-0.12`.
The 0.09 HD pack is unchanged. Release notes belong on the release page, not in
the downloadable asset list. No game assets, personal state, PDBs or test logs
are included in the release packages.

## Ultrawide candidate (November 4, post-0.11)

Automatic aspect uses the selected output's actual ratio at startup, bounded
from 4:3 to 32:9. Explicit 21:9 and 32:9 choices join the existing 4:3/16:9
choices. Changing the camera aspect requires restarting; the running renderer
and guest camera retain the same ratio until then. Common ultrawide resolutions
are included in the Graphics list. Windows and Proton use the same implementation.

The original 640x480 guest coordinates remain intact. The display root's pixel
aspect, logical viewport fit, perspective projection and final presentation now
agree. The first live 2100x900 test exposed an omitted logical-fit step: 8BF4E
still passed 16:9 to A5E80, producing viewport X=76, width=488 instead of the
full 640. Native menu drawing then gained an extra offset and clipping. The
correction pins that exact original call site and passes the startup aspect;
other viewport-fit callers and safe-area operations retain their original logic.
The fixture executes both the original and corrected constructor/fit sequence,
then the original split derivation and perspective builder at five ratios.

HUD artwork retains height-based proportions with corner anchors across the
wider view; projected interaction markers and reticles stay tied to the camera.
Native and replacement menus share a centered 16:9 safe area and matching mouse
coordinates. Frontend background movies use that same safe area. Campaign movie
Fill/Original behavior remains proportional, with the existing audited row trims
and residual bars where necessary. Authored in-game cameras inherit the wider
root; co-op cinematics retain their existing single full-screen presentation.
No new HD texture pack is required.

The native news-film clock uses a nested 4:3 drawing scope, rather than the
menu's 16:9 safe area. The live 21:9 check exposed the clock outside its red
movie box; the corrected 21:9 and 32:9 runs keep it inside that box. The movie
quad has its own framing path. Original movie-update bytes are pinned, and the
fixture checks nested scope restoration through 32:9.

Evidence is under `build/ultrawide-20261104`, including the preserved runtime and
symbols, maintained-source hashes, original-byte guards, native-code fixtures,
screenshots and a private test package. Final runtime SHA-256 is
`0839b483e49223d7cb23155ec05b5370f39714d4e76350ff2874499336659897`.
All 172 maintained build/check steps and four additional asset checks passed.
These include 600 original-guest FMV cases, native root/fit/split projection,
projected prompts/reticles and mouse-menu hitboxes across ultrawide ratios.
The Linux wrapper suite passed its Windows-host checks (19 cases, three
platform-specific skips). The same maintained runtime is used by Proton;
Linux/Proton gameplay remains untested, not inferred from Windows success.

Windows live checks used isolated windowed outputs at 2100x900 and 2400x675
(21:9 and 32:9), not physical ultrawide monitors. Profile/main menus, native
level selection, news-film framing and its clock were inspected at both ratios.
The final 21:9 build loaded single-player Canals with a full-width world view,
centered reticle and corner HUD, then opened pause, Options and Graphics by mouse.
At 32:9, co-op character previews, mouse Ready/Deploy, Hope Street's shared
opening cinematic, transition into both split views, HUD corners/divider and
pause/restart confirmation were checked. These are scoped visual checks, not a
full campaign playthrough, controller gameplay test or performance benchmark.
Full-screen monitor switching and live Original-mode playback remain untested;
both FMV modes and multiple output sizes pass the native-code fixtures.
The live evidence record distinguishes final-build captures from earlier
negative witnesses. Published packages and the user's normal installation are
intact; all 338 tracked mutable files in the source test package were unchanged.

## Proportional FMV Fill (November 4, post-0.10 candidate)

FMV scaling now matches Darkwatch's recent proportional presentation: Fill is
the default and fits the active movie picture after trimming verified encoded
black rows. Original aspect ratio (formerly labeled 4:3; saved value 1) retains
the entire encoded frame. Both preserve proportions. Existing saved choices
remain respected. Residual bars are intentional where further enlargement would
crop the scene; full-frame 4:3 news videos retain side bars on a 16:9 display.

All 42 original Xbox XMV files were decoded through every frame, including the
PAL variants and four menu/background movies. Exact first/last nonzero RGB bytes
were measured, rather than treating cropdetect's line averages as pixel bounds.
The 98,768 decoded frames agree with an independent XMV packet/frame census.
Fourteen non-background films have removable rows. The maintained name table
retains four black rows beyond the all-frame picture bounds and never trims
horizontally. News footage, unknown identities and unsupported same-name custom
replacements are not inferred from Darkwatch's assets. Spanish voice imports
preserve the original video bytes, so the same verified layout applies.

`prepare_xbe_fmv_layout.py` pins the original movie open/close/draw routines and
adds UV adjustment at the four original texture-coordinate submissions, alongside
the existing quad positions. The same original draw fixture verifies 240
movie/mode/aspect combinations at four output sizes, source UV limits, original
guest state, black-band clear ordering and lifecycle/invalid-input guards.
Movie backgrounds and the alternate authored 448-line path remain untouched.
The two active frontend movie callers pass zero for that alternate-height flag.
Fill/Original, save rollback, default Fill, English/Spanish UI and skip routing
are covered by the existing production fixtures. Source-frame comparison images
were inspected; these are previews of the framing, not live game captures.

Audit scripts, per-movie hashes/bounds, independent census, preserved prior
runtime/symbols and focused build evidence are in `build/fmv-proportional-20261104`.
The shared Windows/Proton runtime compiles and focused checks pass. The Linux
wrapper suite passes its Windows-host checks (three platform cases skipped).
New live Windows and Proton movie playback remain unverified. Original movies,
published packages and the HD pack are unchanged; this is not a new release.

## Stored health-pack option (November 4, post-0.10 candidate)

Options -> Gameplay now saves `stored_health_packs` (On by default). This is
one campaign rule shared by single-player and both co-op players. The native
profile difficulty at 643E0()+48 determines inventory capacity: Rookie 3,
Veteran 2, Elite 1 and unlockable Terror 1. The mission-start grant remains one.
The inventory HUD publishes the same effective capacity. Medic stock remains
finite and a handoff consumes one of the medic's supplies.

Off hides the stored-pack HUD, disables pack input/partner healing/reviving,
and restores the original direct medic injection for an injured recipient.
The existing per-player medic owner and stock protections remain active; Off
must not redirect a P2 request to P1. A pending partner injection cannot deliver
a revive or fall through into self-healing after the option is disabled.
Dormant inventory is retained while Off, so switching cannot refill spent packs.
Existing checkpoint companions up to three packs remain readable; restored
counts clamp to the current difficulty without altering the original save.

The shared Windows/Proton runtime uses `xbe_health_settings.cpp`, registered
through the maintained builder's game settings table. `prepare_xbe_health_packs.py`
pins the original profile lookup and difficulty access. Production health,
original medic delivery/eligibility/stock and checkpoint fixtures cover capacity,
Off behavior, owner isolation, cancellation and old records. The real ImGui
fixture checks mouse/controller access, both pause owners, three resolutions,
Spanish strings and failed-save rollback. Evidence and the preserved prior
runtime/symbols are under `build/health-pack-options-20261104`.
No live gameplay acceptance or Proton gameplay validation is claimed for this
candidate. Published v0.10 packages and the HD texture pack are unchanged.

The follow-up health-system audit found that Off still exposed the co-op revival
countdown despite disabling treatment. `xbe_coop_death.cpp` now rejects revival
with packs Off and queues the normal mission failure after the actors update,
including when Off is selected during an existing countdown. Both players alive
does not fail; re-enabling cannot reverse a committed failure. With packs On,
the existing ten-second timer, pause/cinematic freeze, both-down failure and
revived equipment visibility are unchanged. The Gameplay page explains this
in English and Spanish.

Expanded original-code fixtures cover pending medic handoffs across On/Off and
difficulty changes, correct recipient/finite stock, and checkpoint restoration
while disabled. Health, controller/keyboard routing, medic orders, menu saving,
death, downed-camera and blur fixtures pass. The pre-fix death implementation
fails the new Off regression check. Shared-runtime build and evidence are under
`build/health-system-audit-20261104`; the Windows-host Linux wrapper checks pass
with three platform-specific cases skipped. This remains offline verification,
not new live Windows or Proton gameplay acceptance or a published package.

## Stereo / surround output (November 3, 0.10 preparation)

The Audio menu now saves `sound_output`: Stereo (default), 5.1 Surround or
7.1 Surround. This is shared output for single-player and co-op; individual
players do not have independent physical audio devices. The existing volume
rows keep their original callbacks. Mouse, keyboard and controller navigation
reach the added row; English and Spanish strings are included.

The implementation traces Darkwatch's maintained XboxXBERecomp `audio.h`,
`xaudio2.cpp` and `dsound.cpp`, plus its October 31 surround/guest-route audit.
It reuses endpoint capacity detection, fixed-width floating-point output,
requested-versus-active mode fallback, and power-preserving front/rear and
7.1 side/rear distribution. Darkwatch's native XACT addresses and DSP-state
patches do not apply to Urban Chaos and were not copied. Urban Chaos retains
its coordinate-based DirectSound attenuation, cones, pitch and owned MIXIN
chains. Direct speaker bins can address centre/LFE; positional enhancement
does not invent centre/LFE content. Stereo movies are not synthesized into
surround. This is host surround output, not a claim of original Dolby encoding.

The XAudio2 master channel mask determines startup capacity, with source-voice
fallback 8 -> 6 -> 2. The requested mode remains saved independently. Mode
changes reroute the existing output without restarting voices or losing queued
audio. Changing the operating system's device speaker layout requires a game
restart. Unsupported layouts safely fall back; the Audio menu shows active
output as well as the selected preference.

Maintained preparation is `scripts/prepare_xbe_audio_surround.py`, after audio
loop tracing/decode caching. Fixtures execute the production mixer/output worker
and real ImGui menu: exact stereo PCM/ADPCM parity, frame bounds, stream timing,
361 directions, routing energy, centre/LFE bins, nested MIXINs, save rollback,
14 endpoint/failure cases, queue lifetime, and mouse/controller mode changes.
Existing pitch, stream, loop, routing, decode-cache, cone, factor, rolloff and
configuration checks also passed. These are not physical speaker measurements.

Private runtime `1a053b76a74451a5702eefd7c30a7d728d0dbdfaa9ff972c7d3d6f5c58980bce`
was staged with matching symbols and validated launcher manifests; all 16
save/config files retained their hashes. The user's subsequent "okay it all
works" accepts this test build. No speaker layout was specified, so do not
infer physical six/eight-channel validation. The independent visual check was
stopped with Escape and was not completed. Records are under
`build/audio-surround-20261103`; final release checks under `build/release-0.10`.
Windows user acceptance, offline checks and untested Proton gameplay remain
separate. Both packages use the same runtime and existing 0.09 HD pack.

## Angel Heights / NPC ragdoll follow-up (November 3; Daniels user-confirmed)

### Daniels: captured out-of-bounds death and spawn correction

The user reached the missing officer with private runtime `8fda4bab...`, then
restarted at 60 FPS and confirmed he appeared. Read-only captures in
`build/ragdoll-recovery-20261103` establish actual actor creation in both failed
runs (`daniels-live-150910.json`, `daniels-live-151228.json`): template 153 had
health zero, X=4654.6675 cm, Y about -100068 cm, and death reason +150=`0xb`.
The original 38C42–38C66 branch kills an actor below Y=-100000 and sets exactly
that reason. This is a fallen/dead officer, not merely invisible rendering.

The successful 60-FPS comparison (`daniels-watch-151652.json`, 2687 samples,
60 seconds, no read errors) kept him at X=4563.7065, Y=99.99854 with positive
health. `daniels-live-151755.json` subsequently shows higher health and a new
position, consistent with healing and resumed movement. This does not establish
whole-mission completion. The failed samples were roughly 175/209 FPS; the
successful median world delta was 16.98 ms. No settings or live guest state were
written by the observer.

`scripts/verify_daniels_spawn.py` checks the exact ENT/GEO hashes and original
collision triangles. Daniels' authored X=46.094282m lies between two wall layers
at X≈46.086950 and 46.099998m. His original .35m capsule intersects both;
the corridor floor ends at X=46.099998m. The failed X=46.546675m is beyond that
floor, whereas the successful X=45.637065m is inside it. The first transient
collision step has not been recorded; the intersecting start, opposite-side
outcomes and out-of-bounds death are established separately.

The correction changes only this unique CharacterCivilian starting position to
X=4558cm before original initializer DE380 passes the actor transform to
2B15D0 for animation/collision creation. Y/Z, orientation, health, behaviors,
medic interactions and mission conditions remain original. The guard requires
the original character vtable and all three exact authored position words;
a census of all 27 level ENT files finds only this character match. The new
position clears both wall layers by over .50m and retains floor support.
It is a targeted placement repair, not a general change to character physics.

The maintained preparer pins original and generated initializer code, transform
access/copy and capsule dimensions. Its fixture executes the original initializer
up to the real transform handoff (collision construction is an explicit stub),
with guard, idempotence, memory, CPU, MXCSR and invalid-span checks. Those tests,
the 864-pair ragdoll recovery fixture and force-integration fixture all passed.
The compiled normal runtime is
`4d2a582217c64a223cae29281169e8ac225ef3ad128b59a79b2e801ed5055b16`,
with matching PDB
`e2f4cfe4a79c5c00481888b3e3220695b7520abb74feccaf469c0592facd12d0`.
`build/angel-heights-20261103/spawn-build.json` records source hashes and checks;
`spawn-staged.json` records both matching executable copies and successful
launcher manifest validation in the existing November 1 private test package.
All 16 save/config files retained their hashes. Previous runtime/symbols and
manifests are preserved under `spawn-before` and `spawn-install-before`.
The user confirmed "yep it works" after the high-FPS Angel Heights rescue test
requested for PID 38480 on this exact runtime. Record this as user acceptance
of the encounter, not an independently captured post-fix physics trace. The
requested range was 120–240 FPS; achieved FPS during the rescue was not sampled.
`spawn-user-acceptance.json` retains the report and links the launch identity.
Co-op, whole-mission completion and Proton verification remain untested.
The same source correction is in the shared Windows/Proton runtime pipeline;
no release has been published for this correction.

### Earlier investigation and ragdoll evidence

GitHub issue #21 reports the injured officer missing above 60 FPS in Angel
Heights and appearing when capped at 60, with progression blocked. It was read
through the GitHub API on November 3; there were no comments. The user also
relayed that the mission fails when the medic reaches the missing officer.
Keep these as reporter evidence: the earlier 0.06 report also described missing
Daniels at 60 FPS, so the cap is not an established universal workaround.

`build/angel-heights-20261103` retains the issue and a repeatable read-only asset /
instruction audit. In `9-01.ent`, ordinal 153 is `z1 Cop Rescued Curtis`, a
CharacterCivilian using the cop model. Its 197 -> 196 -> 195 -> 194 behavior
chain includes DisallowDeactivation, scripted setup, heartbeat and InjuredBehaviour.
The corresponding text bank identifies Curtis Daniels / Daniels P.D.; the
internal Curtis name is not a different officer. Injury update 17DDF0 already
multiplies its configured health-loss rate by elapsed world time from 1F9010.
Original/prepared bytes match for the inspected routines and active generated
arithmetic was inspected. The authored injury payload has **zero** passive health
loss; heartbeat init 116B00 only lowers health to 40% if it was higher. These
authored settings do not explain disappearance through a draining injury timer.
Friendly wave 233 creates actor 153 after nearby trigger 0; absence at the level
spawn is therefore not evidence of a failed officer spawn. The audit retains
the explicit later retirement commands separately in
`build/angel-heights-20261103/daniels-script-references.json`.
At that stage no failed encounter had been captured. The later live comparison
and placement candidate above supersede that uncertainty; no forced spawn or
mission-success override has been introduced.

The user also rejected the finite-force candidate as a complete fix for excessive
ragdoll launch height, specifically observing it when NPCs kill other characters.
Retain the earlier force-integration tests; that candidate alone did not resolve
the symptom. The later recovery correction and user acceptance are recorded below.
The November 3 comparison below now separates initial velocity from the combined
physics response. Original NPC update 2AB250 computes the velocity supplied at death as
the final position change divided by frame delta, including collision correction.
NPC ragdoll transition 2AC0D0 passes that velocity to 2A6790 and destroys the old
character collision proxy. The inspected NPC hit-force call sites install their
forces once; force application already multiplies by elapsed time and inverse
mass. Contact recovery and joint recovery are not yet individually isolated.
There is no evidence that this and missing Daniels share a cause.

Private runtime ed6bb412... initially encountered an Escape stop (PID 16476).
The user subsequently authorized resumed control. PID 25024 reached Angel
Heights gameplay at roughly 206–220 FPS, but not Daniels' corridor. The snapshot
`daniels-live-130124.json` records the start area; it is not a failed rescue
capture. The user is unavailable to navigate the corridor.

Private PID 19932 (born 2026-11-03 13:17:11 -08:00) then ran two recorded
Canals restarts: the existing 240 cap achieved roughly 120 FPS, followed by an
observed 60 FPS cap with all other graphics options retained. The bounded
read-only sampler captured two newly created opening-combat bodies in each
restart, with no read errors. Median world deltas were about 8.2 ms and 16.6 ms.
Their maximum sampled speeds were 3.07/3.09 m/s at the higher rate and
3.64/2.66 m/s at 60. Rise above the first sampled body position was at most
0.039 m at the higher rate and zero at 60; the reported large upward launch was
not reproduced in this scene. RNG, death poses and exact initial impulses were
not pinned; these 20-Hz observations do not prove frame-rate equivalence or
exclude a different NPC-hit/explosion failure. The prior interval captured only
already settled corpses and is not used as a motion comparison.

`build/angel-heights-20261103/canals-motion-comparison.json` names and hashes the
two raw captures and records those limits. The test game was closed normally;
its 12 private save files and two configs were restored to their pre-comparison
hashes, with post-test state archived under `live-compare/hdd-after-1339`.
No runtime was replaced, rebuilt, or published and no Proton gameplay was tested.

The user then prioritized ragdoll verification before the Daniels script. A
private observation build is recorded in
`build/angel-heights-20261103/probe-build.json` (runtime `6be70c3e...`). It observes
the original 2A5F00 activation and both sides of 2ED370's world step, preserving
guest memory, CPU context and host MXCSR. The fixed recorder holds 65,536
80-byte rows, at most 16 tracked bodies and eight simulation seconds per body;
it records overflow and foreign-thread rejection. There is no file I/O or
allocation on the observation path. The helper fixture checks unchanged CPU,
RAM and MXCSR, invalid spans, owner mismatch, zero delta, capacity and thread
rejection. This qualifies the observer, not a physics correction.

`read_probe.py` checks the private executable, PDB, process path and creation
time before reading only the published immutable row prefix. `analyze_probe.py`
separates inherited velocity, changes between physics steps, and changes during
world simulation. The latter still combines gravity, contacts and constraints;
it is not an isolated contact impulse. Prepared generated sources and the main
build output executable/symbols were restored after compiling. The private
installation's prior executable, symbols, settings and saves are backed up under
`probe-install-before`. No release package or maintained physics behavior was
changed by this diagnostic.

The first private launch was rejected because its executable had been replaced
without updating `tools/runtime-manifest.json`. The private manifest is now
backed up and updated as part of staging; the real launcher validator passes.
Directly launching the game without the launcher's asset environment instead
hit the normal setup check. Neither failure was a ragdoll crash.

After the fix, launcher-started PID 30028 (November 3, 14:16:30 -08:00) displayed
the level-selection screen. The user took over testing. Read-only capture
`probe-startup-141804.json` contains 1,162 rows and two body activations with no
overflow or foreign-thread rejection. Initial velocities match the original
game-to-physics axis/scale conversion; the observed physics deltas are roughly
5.4 ms. This is a recorder activation check, not a confirmed reproduction of the
police-gunfire launch symptom. The user has specifically identified police
gunfire rather than firefighter axe attacks. Leave this active user test alone
until they finish; read the recorder before closing it.

The completed comparison uses `probe-clearer-run-142009.json` (51,786 rows,
22 bodies, PID 30028) and the fresh-process `probe-sixty-complete-142941.json`
(8,533 rows, nine bodies, PID 2224). Both have zero dropped rows and foreign-thread
rejections. The intervening `probe-sixty-pass-one-142702.json` filled the recorder
and is **not** valid 60 FPS comparison evidence; the user then restarted the game.
The fresh process had a verified 60 FPS setting and approximately 16.67 ms world
steps. Both used observation-only runtime `6be70c3e...`.

Bodies 19/7 begin at nearly identical physics positions (32.02, 42.94, 0.51 m),
with zero inherited velocity and two finite forces. At high FPS the root rises
2.60 m with peak upward velocity 9.35 m/s; at 60 FPS it rises 0.60 m with peak
1.92 m/s. Net changes during physics steps up to the peak contribute 7.73 versus
0.09 m/s; between-step changes contribute 1.62 versus 1.82 m/s. Bodies 5/3 give
another similar-location contrast (1.82 versus 0.62 m rise). These are separately
played encounters, not identical poses/RNG/contact histories. They implicate the
combined physics response, not a larger inherited death velocity or finite-force
impulse in those examples. Gravity, contacts and joints still share that measurement.

The opening scripted throw adds approximately 6.67 m/s between steps at both
rates and rises similarly; preserve it. The largest late limb speeds in some
records are bodies falling below the world, **not** upward launch evidence.
Original constraint builders multiply positional/angular error by inverse
substep time (contact 34A677; joint kernels including 3464C6). A ragdoll-only
60 Hz recovery-rate ceiling was investigated next; acceptance is recorded below.

After the user closed the game, `restore_probe.py` restored the private runtime,
matching symbols, manifest, settings and saves with hashes checked. Receipt:
`probe-restored.json`; post-test state: `probe-install-after-143139`. The public
release remains unchanged. Proton gameplay remains untested.

### Ragdoll recovery correction and Windows acceptance

`scripts/xbe_ragdoll_recovery.h` and `prepare_xbe_ragdoll_recovery.py` now bound
the inverse-time recovery factor at its 60-Hz value while building ragdoll contact
and joint rows above 60 FPS. Hooks cover limited hinge 2F4AF0, ragdoll joint
31E840 and contact builder 34A3C0. Original body property key 1 / value 4 identifies
ragdolls in either contact order. The helper temporarily replaces context +4,
using 60 divided by the inverse iteration count at +16, then restores its exact
bits. Actual elapsed time, initial death velocity, finite hit forces and gravity
remain unchanged. Non-ragdoll bodies and rates at or below 60 retain their path.

The maintained `test_xbe_ragdoll_recovery.cpp` fixture compares full guest RAM
and CPU state for 864 cases across all eleven authored joints, six frame rates,
four iteration counts and both ragdoll body orders. It checks invalid inputs,
restoration and MXCSR. The original contact solver also reproduces the concrete
failure: identical penetration produces 0.9 m/s upward recovery at 60 FPS but
3.6 m/s at 240; corrected recovery stays at 0.9 m/s across tested rates. This
isolated solver fixture is not a whole-scene simulation. Existing force,
contact-filter (763 cases) and retention/settings checks pass.

`build/angel-heights-20261103/recovery-build.json` identifies the normal maintained
runtime `06f1da739f797d31d658f4788d72077f01bfd10fefeca24f0f21f55e4091774d`.
The observation-enabled private candidate is
`8fda4bab8624973e4d0de028a96e57e4c7c128fcc8cb3f6b906f523b520cf627`, staged with
matching launcher/package manifests under `build/ragdoll-recovery-20261103`.
It records at most three simulation seconds per body into 131,072 rows without
requiring F9. Launcher validation and package file hashes pass. Normal generated
sources and build output were restored after producing the observer candidate.

On November 3, the launcher started PID 28216, born 14:58:19 -08:00, and video
was visually confirmed. Settings retained a 240 FPS cap and disabled VSync.
The user tested police gunfire and reported "it worked", then proceeded to
Angel Heights. `probe-confirmed-highfps-150052.json` preserves 7,603 rows for
nine bodies, with zero dropped rows or foreign-thread rejections; its SHA-256 is
`d6984247a021c29d99262153bf3d132f8bd7962c312184aa8d2f3c5c08dcfede`.
The sibling summary records per-body median world deltas around 6.2-6.8 ms.
The scripted throw retained an approximately 6.65 m/s between-step impulse and
0.82 m rise. The other eight recorded bodies rose at most 0.57 m, with peak
upward speeds no greater than 2.17 m/s. Encounters are not identical RNG/contact
replays, and the final body's three-second observation was not yet complete.
This is user acceptance of the reported Windows police-gunfire symptom, not
full-campaign, co-op or Proton acceptance. The shared maintained correction is
used for both Windows and Proton; no release has been published. Daniels'
scripted rescue remains unverified and is not claimed fixed by this change.

## Medic targeting, blinking and gameplay controls (November 3, unpublished test)

The user's failed first-medic encounter was captured read-only in
`build/gameplay-controls-20261103/medic-live-111119.json`, against runtime
`41fb724c39a0ff9a0bec7fa904c46da8231f489ba1619844f912826e60ae6662`
and its matching PDB. The medic had all three supplies and an active
HealOrderClient. The player's native friendly reticle hit identified that exact
medic, but the orders manager had type zero / no target. The team-medic init
170E80 does not join SELECTABLEITEMS like the ordinary medic init 169700.
Our shared per-player offer guard therefore rejected a valid native offer.

For this active-offer path only, absent orders targets now fall back to C1020's
native friendly camera hit. Exact medic identity, the retained offer recipient,
300-unit distance, available stock and inventory capacity remain required.
Manual orders and scripted treatment locks are unchanged. Both the offer query
and its actual acceptance callback use the same target check. The native
property/command fixture reproduces the captured state for single-player and
each co-op owner, rejects look-away/wrong targets, and checks range, stock and
capacity. Scene collision and text resources remain fixture boundaries; this
is supplemented by the user's November 3 confirmation that the corrected
medic health-pack prompt works in the Windows test.

The same clip showed fast eyelid blinking. Retail modifier 2B7F10 increments
its counter on every pose application, including repeated view passes. The
correction advances only that counter at the original 30-Hz cadence using
animation delta forwarded by 21C34C -> B9830 -> the modifier. The world +8
update stamp deduplicates repeated passes. Existing five-step eyelid poses,
forced-closed eyes, original RNG and matrix calls remain; every fresh pose still
receives the current eyelid state. Destructor 2B5BD0 retires the host fractional
clock. The emitted native-body fixture checks 30/60/120/144/240/360 FPS, two
actors, repeated views, zero delta, forced closure, bounded hitches and reuse.
Matrix math is a fixture boundary. The user also confirmed on November 3 that
the fast blinking is fixed in the Windows test.

This candidate also retains the preceding changes: continuous Hold Aim through
fire/recoil states, an assignable Walk action with Hold/Toggle behavior and
35% movement, and an assignable Toggle HUD action. New bindings start unbound;
their controls and saved settings are separate for each player. Menus, focus
loss, captures and cinematics cancel walking safely. Hidden HUDs keep updating
their game state; subtitles, menus and downed warnings are retained. English
and Spanish menu strings and the production input/UI fixtures are included.

Normal candidate runtime SHA-256:
`ed6bb412d9040e6b35036bde6f9457037fcc7e0c6c2cbeb079c5415042751a73`.
Source/backups, clip frames, original instruction witnesses, build/test logs,
source identities and paired package receipts are in
`build/gameplay-controls-20261103`. Windows and Linux/Proton share this runtime
and shared asset inventory; platform setup is checked separately. No release
was published. The existing 0.09 HD pack remains compatible. Live Windows
acceptance of the medic prompt and blinking is recorded above; the remaining
gameplay changes and Proton gameplay still need separate acceptance. Neither
confirmation resolves the outstanding Daniels or excessive ragdoll reports.

## High-FPS ragdoll report (November 3, candidate implemented)

The user reports bodies travelling farther when shot or caught in explosions.
The camera-push/recoil correction does not change ragdoll forces. The initial
report was against zoom-sensitivity runtime
`953737f282c2d9de6ba3f1c2c3484732db5cf88741b302faefe3ba72c25baa39`.

The active generated path contains a frame-rate-sensitive finite force:
`2A3590` / `2A3760` install the force table `48D9EC`; its update `2A33A0`
subtracts the frame delta before computing remaining/duration. The ragdoll
loop `2A5DA0` updates the force before `2A5C60` applies it. Motion slot +6C
(`2F4FF0` for the witnessed motion table) already multiplies by elapsed time
and inverse mass. This is an end-of-step force sample, not a missing dt factor.
The actor update at E7262–E728F supplies the gameplay delta; physics time is
published by `29E5B0`, with the caller `2145D0` capping hitches at 1/15 second.

`build/ragdoll-fps-audit-20261103/results.json` records original byte witnesses,
active generated function hashes and an isolated binary32 equation replay at
30/60/120/144/240 FPS. Relevant original and prepared-XBE byte ranges match.
For the default 0.6-second directional / 0.4-second upward force durations,
the replay gives +1.43% / +2.17% integrated force at 120 versus 60 FPS and
+2.14% / +3.26% at 240 versus 60 FPS, at a fixed spatial sample and unit mass.
These are force-equation results, not measured body travel distances or execution
of the generated C++. A paired live shot/explosion capture, collision response,
initial velocities and moving force-radius samples remain unverified. Do not
claim this modest difference explains the whole symptom or mark it live-fixed.

The gameplay-controls candidate now integrates the finite force over the
existing 60-Hz envelope and retains its final partial impulse once before
removal. Permanent forces and unsupported inputs keep their original path.
The generated update/removal/application/motion fixture checks four durations
at 30/60/120/144/240/360 FPS against retail 60-Hz integrated impulse. Spatial
falloff and collision stepping are boundaries; actual body travel still needs
the user's in-game comparison. Evidence is in the gameplay-controls directory
above. This is not a global physics or all-frame-rate equivalence claim.

## Weapon zoom sensitivity (November 3, unpublished follow-up)

Controls has a Zoom sensitivity slider (1–200%, default 100%) for each player.
It multiplies that player's existing zoomed mouse/stick look speed only while
the native weapon zoom state is active, including both rifle magnifications
and Hold/Toggle aiming. Unzoomed look, movement, physical pistol-spin/order
gestures, recoil and shot/reticle alignment retain their existing paths.
The native controller delta hook at BFD86 runs after acceleration and before
the original pitch clamp. Actor identity and live gameplay are validated by
the existing aim-mode helper; no global camera-FOV heuristic is used.

Regular sensitivity also displays 100% at its original default: the existing
MouseSensitivity value 400 is shown as 100%, with its full 0.25–400% range;
native stick sensitivity 50 is shown as 100%. Stored values are unchanged, so
existing profiles retain their exact turning speed. ZoomSensitivity is saved
atomically in Aim / AimPlayer2; absent or invalid values use 100%. P2 cannot
overwrite P1's settings. English and Spanish labels are included.

Evidence, source/runtime/symbol backups and matched Windows/Proton test packages
are under `build/zoom-sensitivity-20261103`. Runtime/input fixtures check native
zoom state, both players, 100% parity, fractional low sensitivity, pitch clamps,
menu/save failures and invalid settings. Real-ImGui menu tests cover slider
dragging, keyboard editing, defaults, discard and player isolation. These are
offline checks; live Windows aiming feel and Proton gameplay remain unverified.
The published 0.09 release and HD texture pack are unchanged.

## Per-player reticle colors (November 3)

Graphics > Reticle settings contains size plus separate normal, enemy, friendly
NPC and pistol-spin colors. Each offers Original and nine named colors with a
preview. Color choices follow the existing pause/settings owner, persist as eight
GameInfo options, and publish only after a successful Apply. Invalid optional
values reset only that color; a failed save rolls both players back. Size remains
the existing shared setting. English and Spanish labels are included.

The pinned 738D0 native update still chooses target priority and each owner's
spin state. The new hook at 739F9 replaces only its selected RGB, before the
existing sprite/fade update. Alpha, targeting, spin rewards, geometry, reticle
alignment and shot direction are unchanged. Original bypasses the replacement.
Evidence, prior runtime/symbols and checks are in `build/reticle-colors-20261103`.
Live color acceptance on Windows and Proton remains pending.

The user confirmed the co-op medic prompt works on the November 2 logging
candidate (runtime `4ebc5d7e694cc6b03242c33cc520987996e1a6b07e313509ab388cb79756297e`)
after the November 3 launch. This is Windows user acceptance of that encounter,
not a whole-campaign or Proton verification.

## Opt-in support logs (November 2)

Darkwatch's current `docs/DIAGNOSTICS.md`, launcher worker and release notes were
reviewed read-only. Urban Chaos now follows the same default-off, local-only
toggle/Open Logs pattern, with bounded files and no automatic upload. Windows
Manage, the Steam Python shortcut and the terminal Proton wrapper all use the
launcher's `GameData/.diagnostic-logging` choice, independently of selected game
data. Logs live in that launcher's `GameData/logs`. See `docs/DIAGNOSTICS.md` for
exact scope and Linux commands; this does not claim every Darkwatch probe exists.

The launcher strips inherited developer flags and enables `UCRR_SESSION_LOG=1`
only after opening the opted-in game log. Release Report/Info/Warn and limited
reports then pass through a serialized 64-MiB stderr logger; disabled log macros
do not evaluate their arguments or touch rate-limit counters. Debug, GPU/frame,
heap and audio captures remain compiled out. Startup uses the existing system
and settings report; existing half-second FPS calculation supplies a log sample
every ten intervals. No extra per-draw clocks or readbacks were added. Errors
still reach the existing bounded failure report with support logs off.

Setup actions and the terminal wrapper's Proton output have separate 8-MiB caps
and 30-file retention. Runtime logs retain 30 sessions. Managed paths reject
links and shared file leaves while accepting Proton-mapped host ancestors.
Log-write failures never authorize changing installation state or failing play.
No input/desktop text, screenshots, audio recordings or memory dumps are added.

Evidence and prior source/runtime/PDB are in `build/opt-in-logging-20261102`.
Default-off/opt-in/cap checks execute the production release logger and heavy
capture guards. Worker tests exercise real setup and mocked child output, not
gameplay; launcher tests and Linux wrapper fixtures record their own platform
coverage. Paired packaging retains the same runtime and shared assets. Live
Windows diagnostic-session and Proton gameplay acceptance remain unverified.

## Health-pack input symbols (November 2)

The stored-pack HUD now draws the controller direction-pad icon with its lower
arm highlighted and a compact multiplication-sign/2 badge. Keyboard/mouse uses
the current primary or secondary healing binding in a keycap (or existing mouse
artwork); rebound controller face buttons use the detected controller family.
The shared UI path covers single-player and both co-op view owners, preserving
the native health-bar fade and conversation/menu hiding. Gameplay inputs are
unchanged. Wheel Down is deliberately excluded from menu-icon code lookup:
its keyboard source 257 otherwise collides with the menu's A-button code.

The production ImGui fixture checks actual glyph/atlas UVs for both owners,
primary/secondary rebinding, unbound input, mouse, wheel, controller/default
gesture, original/HD HUD and three output sizes. Existing menu/input/fade checks
also pass. Runtime SHA-256 is
`01e467beed3c0059aca44ad15e05dd245151e22a15456049c6b8b8f7d18c0312`.
Evidence and the prior runtime/PDB are in `build/health-binding-icons-20261102`.
This is offline UI verification; live Windows/Proton visual acceptance of the
new symbols remains pending. Paired 0.09 release preparation includes this change.

## Persistent medic collection prompt (November 2)

The user reported no pack-request prompt for either player at the first medic
in the first mission, before either player collected any stock. Full health was
already admitted by the inventory helper. A saved tutorial-seen flag at profile
+128 still prevented native help routine 140FB0 from publishing its medic hint
after the original timer expired. A negative control reproduces that suppression
with stock available and both owners eligible; the user's actual profile flag
has not been confirmed in a live run.

The pinned branch at 14110E now reaches the existing HealPlayer eligibility query
regardless of tutorial history. Native text, animation and per-player look/range,
inventory capacity, medic stock and busy checks remain in place. Tutorial history
is preserved; other tutorial hints retain their original behavior. The fix also
applies to single-player collection.

Regression coverage includes both view owners and update orders, an already-seen
tutorial, repeated updates, looking away, full inventory, empty medic stock and
the single-player singleton. All eight targeted suites and 56 existing WARP
screen-sampling comparisons pass. The fixture exercises the native medic help
body and real inventory helper; scene queries and text resources are boundaries,
so it does not establish live visibility at the reported first-medic encounter.

Normal runtime SHA-256:
`29f89d268187fbad0f9c0209d003c17f7a616e78b5c809a6a721c7fddc3f3cff`.
Prior source/runtime/PDB, the failing-before regression, successful checks and
paired package receipts are under `build/medic-repeat-prompt-20261102`.
Windows and Linux/Proton use the same runtime and shared assets. The existing HD
pack remains compatible. This is an unpublished test correction; Windows
first-medic gameplay acceptance and live Proton verification remain pending.

## Controller menus and partner healing (November 2)

The user accepted the previous pickup notice and pause-navigation changes, then
reported getting trapped on the original Controller reference page. That widget
calls 2745F0 directly instead of the shared 4C3F0 reader. Its three hardcoded
player-zero query arguments now use the accepted pause owner. Pinned original
query/jump-table replay reproduces the old P2 Back failure and confirms P2 Back,
other-player isolation, and unowned single-player behavior.

Holding X was also opening ImGui's built-in gamepad window switcher, whose windows
are not a game menu. The host already provides explicit controller navigation;
disable only ImGui's generic gamepad navigation. A real-ImGui negative-control
fixture reproduces the old hold-X window target and verifies its absence during
gameplay and both pause owners. Original gameplay X remains mapped normally.
Health overlays also hide for an owned pause when Controller help has no host
menu context. Cheats is now a main-menu-only entry, with no pause entry or Y
shortcut; its native toggle authorization follows the exact main-menu epoch.

Controller health gestures use the physical D-pad Down for that controller's
player only. A double tap within 300 ms requests self-healing. Holding for 650 ms
while facing a nearby injured/downed partner requests a partner injection.
A cancelled or completed hold stays consumed until release. Focus, menus,
cutscenes, capture and disconnection reset the gesture; existing keyboard health
behavior remains unchanged. Keyboard/mouse has a separate Heal Partner control,
default U (unbound on upgrade if U is already claimed), with two rebindable slots
saved as HealthPacks Partner0/Partner1. P2 controller saves cannot overwrite it.

Living-partner treatment reuses the existing partner injection animation. Native
BDAB0 supplies maximum health; the Heal contact queues the recipient's +46C refill,
as CD9D3/CD9D8 do, and D2BB9 applies/clamps it. The giver spends one pack only at a
successful contact. Identity, range, facing, health and pending treatment are
revalidated; cancelled injections do not heal self, consume stock or turn into a
revive. Living partners deliberately show no healing prompt. The existing downed
Revive Partner prompt remains. The native full-health query runs for this feature
only in co-op.

Offline checks cover both giver/recipient directions, original self/refill and
revive paths, animation event exactly-once spending, cancellation, native HUD fade,
checkpoint stock, binding persistence/failure/conflicts, real input ownership,
physical gesture timing/handoff, and menus in English/Spanish at three sizes.
The accepted pickup and aiming tests still pass, including all 56 WARP
screen-sampling comparisons. The PC prompt input preparation is now a reusable
portion of the existing preparer; both full builds and this incremental build
retain the identical prompt/device handoff transformations.

Normal runtime SHA-256:
`940b20209cb9fb148a05b73f4419b600692a6236cf8f90e50eef1d341b8ba171`.
Source pins, prior runtime/PDB, checks and matched Windows/Proton test inventories
are in `build/coop-controller-health-20261101`. Both archives have identical shared
files, verified member hashes and no personal state. Existing HD textures remain
compatible. The private test installation was updated with both runtime copies and
its manifest; its ten saved-state/config/texture-manifest files remained unchanged.
No release was published. See live acceptance records in that evidence directory;
automated checks do not establish live controller timing or Proton compatibility.
Windows launch displayed intro video, and a later agent observation showed both
co-op views and the separate H / Down x2 health HUD labels. Gameplay had advanced
without agent navigation, so no further input was sent. This is boot/render
evidence only: the new healing gestures and menu fixes still need live acceptance;
Proton remains untested live. Details are in `live-acceptance.json`.

## Co-op pickup notices and Player 2 pause (November 1)

After accepting the co-op aiming correction, the user reported that Player 2's
weapon/ammunition pickup displayed the larger text popup on Player 1, and that
Player 2's controller could open Pause but could not navigate it.

The inventory grant and small ammo indicator already belong to the collecting
actor. The larger notice instead goes through C51D3, the global type-15 channel
query (4AB90), and the five-row native FIFO (4A290/4A3A0/4BA80). Collector ownership
is now scoped only around that producer call. A five-row sidecar follows native
insertion, overwrite and expiry. The original renderer temporarily receives the
owning player's Y anchor, restored after drawing; text, animation, lifetime and
inventory remain native. Unrelated checkpoint messages retain their behavior.
Stale actor/world identities and cinematic presentation suppress personal notices.

The host frontend previously admitted an owned controller Options screen but not
the rest of its Pause flow. It now admits the owner's Pause, confirmations and
Objectives, retaining native callbacks and separate Controls/FOV settings. Native
foreground suppression is gated on the accepted host screen, preventing duplicate
menu labels and prompts. The controller Y shortcut continues to open Cheats.

Offline tests exercise the original pickup producer/query/FIFO/draw path, including
a negative control reproducing Player 2's old placement, both owners, FIFO shifts,
overwrite/dedup/full queue, stale identities, cinematics and single-player isolation.
Frontend tests cover Player 2 navigation and restart/checkpoint/quit confirmations,
Objectives, Controls and Graphics at three resolutions in English and Spanish.
Input ownership, reward routing, view layout, release exclusions and all 56 existing
screen-sampling pixel comparisons pass. Pickup text fixtures replace font metrics
and draw submission; they do not verify rendered glyph pixels.

Normal runtime SHA-256 is
`ddac219536ddb328e334ecdeb3d86068be61ef1aae6ce7912118181c395ba956`.
Backups, checks and paired package manifests are in
`build/coop-pickup-pause-20261101`. Windows and Linux/Proton test packages share
the same runtime and 323 shared files; launcher/setup checks and archive member
hashes pass. Packages contain no personal state and remain unpublished. No new HD
texture pack is required. Live Windows acceptance of these two fixes and live
Proton verification are still pending.

## Scene-copy alignment and co-op candidate (November 1)

The successful private capture in
`build/reticle-sight-20261101/live/20261101-210214` separates the remaining aiming
offset from ballistics. The uploaded camera projects the actual decal geometry
at the logged CPU position. Four later one-to-one screen passes each subtract
half a guest texel from their texture coordinates, shifting the scene two guest
pixels right/down before the HUD is drawn. At 2560x1440 that is 8x6 display
pixels. The decal artwork and its normal offset explain smaller differences;
they do not explain this cumulative scene displacement.

`prepare_xbe_screen_sampling.py` now admits the actual three-MOV passthrough
program, matching float position/UV geometry and same-sized rectangular render
textures. Only those stages omit the extra half-texel bias. Authored filtering,
downsampling, transformed UVs, depth aliases, and extension-owned draws retain
their existing behavior. Bullet direction, spread, weapon pose, and the earlier
reticle centering are unchanged. The user confirmed the single-player correction
on diagnostic runtime `fe7687bb787558c3a1f4b3d0b0e0f1bfcadc7d643ff33a2201c140cc8756f75c`;
its two saved frames show four admitted passes each. Evidence and feedback are
under `build/reticle-sight-20261101/live/20261101-212837`.

The co-op follow-up removes the unnecessary full-guest-viewport restriction.
The host viewport already covers the target; the guest viewport controls its
scissor. A copy may therefore cover the full target clipped to one player, or
use that player's exact rectangle with absolute framebuffer UVs. Both forms
are admitted without changing viewport/scissor state. Shifted, malformed or
local-zero UVs in the lower view remain rejected.

The production-derived test uses the captured shader's exact translated HLSL,
actual constant generation, and WARP sampled pixels. Forty four-pass comparisons
cover full screen, both clipped halves and both local quads at 0.5x, 1x, 1.5x and
3.75x scales. Corrected samples retain the original image; the other viewport
is byte-identical. Negative controls reproduce the old two-guest-pixel shift.
Co-op viewport/effect/cinematic, fog, depth-alias, HUD, reticle and release checks
also pass. All generated game logic remains byte-identical to the accepted
single-player candidate. Production runtime is
`f5b09484e43bee017bbcd3ac9bb3c5c42574ed07fef7b864cb9b738f2f72e652`.
See `build/coop-screen-sampling-20261101` for source/runtime backups and checks.
The GPU fixture does not establish live co-op or Proton acceptance; record those
separately. These candidates are unpublished.

The user rejected that first co-op candidate as "still off by a bit". Both saved
frames in `build/reticle-sight-20261101/live/20261101-214209` show two corrected
copies followed by two missed copies per player. The latter reset the guest
viewport to full screen while retaining absolute upper/lower-half geometry.
Their remaining half-texel biases sum to one guest pixel right/down (4x3 display
pixels at 2560x1440). Both players fired in the log; the automatic impact/settled
captures belong to the first player's shot and contain both views' rendering.

The follow-up admits either exact half-target quad independently of the guest
viewport, retaining the original shader/format/UV/resource guards. A malformed
quad spanning mixed corners remains rejected. The production-derived WARP test
now includes the captured two-half-viewport/two-reset-viewport sequence for both
players at four scales. All 56 comparisons pass: the previous candidate's
negative control reproduces the residual shift, corrected copies retain the
original pixels, and the untouched half remains byte-identical. The 59 rejection
cases and existing scoped renderer/co-op/release checks pass. These pixel tests
exercise identity sampling, not the full authored tone/history color blend.
Generated game logic remains unchanged. Runtime is
`7121a5504210eebe63c7a7867a6df6b0b12300709d18e0fa5b028caeca2842cd`;
backups, captured-state analysis and checks are in
`build/coop-screen-sampling-reset-20261101`.
The user confirmed "yeah that fixed it" on the Windows co-op diagnostic runtime
`a0cd1f08b628584ccc2865f3edffc132ee0ca89299ac341d62e433db2e94e5ca`.
Both saved impact/settled frames show all eight identity copies aligned, including
the four previously missed draws and both history-texture stages. Evidence and
scoped user acceptance are in
`build/reticle-sight-20261101/live/20261101-215600/user-acceptance.json`.
This accepts the reported co-op alignment issue, not every weapon or campaign.
The maintained correction is shared by Windows and Proton; Proton has not been
tested live here. Do not treat older paired reticle ZIPs as this runtime.

## Single-player reticle comparison candidate (November 1)

The user reports the aiming mismatch in single-player and asks whether the
reticle, rather than the bullet, is misplaced. Source replay of the original
pistol sprite, widescreen layout, 80% size and signed-sixteenth queue places its
aiming texel at X319.269043 instead of X320: about 2.924 pixels left at 2560 wide.
This is source analysis, not a new live GPU measurement. The already-qualified
co-op reticle centering did not apply to single-player.

The comparison candidate admits the validated single-player HUD owner
to the same horizontal centering path. It retains native vertical placement,
scope masks and unrelated HUD layout. It also removes the two compensating
co-op bullet-bias calls, restoring original directions for this comparison.
This supersedes the nine-pixel calibration only in candidate source; that
calibration's historical user acceptance below remains valid evidence. The
larger discrepancy and zoomed iron-sight alignment are not yet explained: weapon
geometry uses an independent projection and Weapon FOV. Do not call the whole
reported aiming issue fixed from the small reticle error alone.

The production-derived fixture passed 1,680 placement cases, including both
co-op owners, single-player, original/HD aiming texels, unrelated owners and
unchanged vertical/cinematic placement. Its SP pistol replay reduces the 2560-wide
horizontal error from -2.923828 to -0.050781 pixels after native queue conversion.
The existing 1,208 reticle-scale checks and weapon/ladder/taser/projected-HUD
checks also pass. Diagnostic-enabled/disabled and release-exclusion checks pass.
Generated firing code differs only by removal of the two calibration calls and
their unused header; other generated units and the accepted recoil are unchanged.

Source backups, replay, matching prior executable/PDB and verification records
are under `build/single-player-reticle-20261101`. Candidate runtime SHA256 is
`c6b9308da398cb90f465aa5a43c49db2bbaf10e78700dde08492d91e53d9cf02`.
Windows and Proton use that same comparison runtime. Earlier menu/FMV packages
remain available. Live aiming, zoomed pistol sights and Proton gameplay still
need acceptance; these source/fixture checks do not establish GPU pixel alignment.

## Main menu and full-frame FMV candidate (November 1)

The main menu hides the original Xbox Live row by its byte-pinned localization
key and inserts Co-op directly below Single Player. A visible-to-native row map
preserves original callback identities and disabled rows. Both keyboard/mouse
and controller use this main-menu list; original profile and campaign screens
still retain their existing input ownership. Other menus are unchanged.

The user explicitly chose full-width stretching over cropping or side bars for
4:3 FMVs. Fill now presents the complete original movie quad, including titles
and news tickers; 4:3 retains original proportions. Baked-in movie borders remain.
English and Spanish settings descriptions explain this distinction. This
supersedes the earlier proportional zoom behavior described in the old evidence.

The production ImGui fixture covers removal/reordering, native action identity,
disabled entries, controller navigation and co-op entry at three resolutions in
English and Spanish. The actual generated movie-draw fixture checks full source
coverage, proportional 4:3, UVs, border clears, guest state and lifecycle. Recoil,
frontend, co-op menu, graphics settings, language, intro-skip and release checks
also passed. The accepted high-FPS recoil implementation remains unchanged.
Evidence and matched unpublished Windows/Proton packages are under
`build/menu-fmv-20261101`. Offline checks do not establish live movie or Proton
acceptance; consult that directory's verification records for observed coverage.

## High-FPS recoil correction candidate (issue 19)

The user resumed issue 19 after observing excessive firing shake in the November 1
test. The connected CameraPush integrator at 45C00 restored/damped velocity with
elapsed time but added that velocity to position once per update. The maintained
`prepare_xbe_camera_push.py` now applies a fractional step of the original 30 Hz
spring response before the unchanged angle bend and Euler-matrix publication.
At exactly 30 Hz the original routine is untouched. Each call uses the current
component's state; there is no shared co-op history or recycled-pointer cache.
Shot impulses, pulse lifetime/fade, ammunition, bullet alignment and firing rate
are unchanged. This is a title timing correction in the XboxXBERecomp path, not
a reintroduction of the retired native CameraPush implementation.

`test_xbe_camera_push.cpp` executes the byte-pinned original and actual patched
generated integrators and their original matrix builder. It passed 4,780
comparisons, including 1,000 bit-identical 30 Hz states, repeated impulses at
30/60/120/144/240/360/1000 FPS, stack/register and surrounding-memory guards,
independent owners, zero delta, hitches and settling. Maximum aligned state or
matrix difference from the 30 Hz reference was 2.38418579e-7. The sample angular
impulse peaked at 0.14 radians at 30 FPS, 0.302226 uncorrected at 120 FPS, and
0.142403 corrected at 120 FPS; the small peak difference is finer time sampling.
The original nonlinear soft limiter is interpolated for extreme pushes; its
cross-rate trajectory is not proven identical. Sustained non-weapon effect
forces and live feel across all weapons remain outside this scoped comparison.

Evidence, preserved previous executable/symbols, runtime and paired package
identities are under `build/issue-19-camera-shake`. Windows and Linux/Proton test
packages use the same corrected runtime and shared assets. This candidate is
unpublished. On November 1, 2026, the user confirmed "yep that fixed the recoil"
after the Windows test of runtime
`3825d26425cd89c0f586f2cab8808f133cd40a1990e93c235051bb9114eca509`.
This accepts the reported firing-shake fix; the feedback does not specify every
tested frame rate, weapon or co-op player. See `user-acceptance.json` in the
evidence directory. Proton live validation remains pending.

## HD game texture coverage audit

The separate unpublished 0.09 release download is
`UrbanChaos-BurnerEdition-0.09-HD-Game-Textures.zip`. It promotes all reviewed
Coverage Test 3 texture payloads unchanged, with updated credits/install notes.
Fresh installation and upgrades from 0.06 pass with both packaged workers on
Windows; this does not establish live Proton or in-game texture acceptance.
See `build/release-0.09/hd-pack-20261103` for the archive/installer receipts.
The published 0.06 pack has 1,238 exact GPU mappings; the first coverage test has
1,402; the second has 1,653; the third has 1,803. All mappings and payloads from
all earlier ZIPs remain byte-identical. The third sweep adds 150 GPU identities,
204 distinct Xbox texture records and 708 archive/name references. Totals are
1,800 payloads, 2,178 texture records and 6,931 references. This covers 78.73% of
the 2,290 supported gameplay-only GPU identities, not 78.73% of visible pixels.
Shared UI resources and 34 unsupported-format records are outside that count.
Both Windows and Linux/Proton use this same optional ZIP and manifest; no
executable, launcher, game archive or installed user assets were changed.

The audit scanned all 133 original ENT/GEO/LST archives, which still match the
supported disc manifest, and verified all 2,435 DDS files against the original
xXthe.RockoXx RAR extraction. The old download contained every previously
accepted replacement; missing coverage came from selection, not lost packaging.
The complete Xbox inventory has 3,139 texture records / 12,037 references.
It adds tiny images, `.tga=...` suffixes, and the two legacy archive directories
missed by the earlier scan. Bump maps and two uncompressed, opaque sight
reflection textures stay outside the ordinary compressed-color replacement path.

`convert_ps2_world_pack.py` now distinguishes UI path components from gameplay
names such as `sis_button` or `map_41_menu`, removes byte-identical PS2 candidates
before testing ambiguity, and compares RGB independently of PS2 alpha. New
images resize RGB first, then attach the Xbox alpha, retaining complete BC3
mipmaps, matching aspect ratio and the existing 4x / 2048-pixel size bounds.
A separate visible-alpha comparison prevents hidden RGB from qualifying a wrong
visible detail. The whole-inventory GPU identity deny set protects shared UI
resources across different archive headers, including `font_untextured` and
world `white`/`untextured` aliases. Never replace that fence with per-name checks.

The first candidate contained 145 automatic additions plus 34 source-hash-pinned pairs
in `scripts/ps2_world_pack_reviewed.json`, inspected against the original image
with Xbox alpha. Those pairs bypass only the coarse 16x16 selection threshold;
fine RGB, visible-alpha, size, aspect, original-source and UI checks still apply.
The near-black candidate and cross-aspect hypotheses remain unresolved. Fourteen
previously accepted records fail the newly stricter image checks; this additive
candidate deliberately retains their existing approved payloads. A future full
regeneration must review those records rather than silently removing them.

The second sweep tests all eight image orientations, exact fine-resolution
comparisons over all eligible source files, alpha-only hypotheses, identical-RGB
Xbox variants and nine standard atlas crops. It also resolves all 1,998 animated
texture references and verifies current generated co-op assets introduce no
unlisted texture identity. The decisive omission was PS2 images with swapped
axes: 245 GPU identities (300 archive-header variants) pass the original coarse
ambiguity and detailed fidelity checks after Pillow `TRANSPOSE`. Six additional
8-pixel-high strips pass a stronger fine comparison; resizing them to a square
16x16 descriptor had rejected them. Every added pair was visually compared.
The maintained review manifest pins original and source hashes and an explicit
optional `transpose`; transformations occur before fidelity/aspect checks.

After the second sweep, 637 supported gameplay identities lacked an approved mapping:
536 diffuse/color-effect images, 24 alpha-shape or near-uniform images, 53 tiny
strips, 22 solid helpers, one transparent helper and one legacy candidate.
These are unresolved matches, not proof that their source art is absent.
Alpha-only PS2 candidates often have different alpha amplitude; copying that
channel can weaken effects, while retaining Xbox alpha adds no new mask detail.
Three weapon/hand variants with covered base RGB fail the visible-alpha guard.
No atlas crop was accepted: apparent matches were unrelated gradients or blood
colors whose shapes came entirely from the retained Xbox alpha. Do not bypass
these material checks to increase coverage counts.

Evidence and reproduction scripts are under `build/hd-texture-coverage-audit`:
`inventory/`, `candidates/`, `runtime/`, `convert_additions.py`,
`convert_manual.py`, `package_test.py`, and `package-verification.json`.
Second-sweep evidence is under `second-sweep/{orientation,alpha,atlas,catalog}`,
with conversion/package scripts, `package-verification.json` and
`final-coverage.json`. `releases/HD-Texture-Coverage-Comparison.csv` compares
published/first/second coverage; `HD-Texture-Coverage-Remaining.csv` lists each
remaining identity, names, archives and review status. Category counts can
overlap where identical GPU data has names in multiple categories.
The converter regression, installer rejection/rollback regression, existing
native world-texture fixture and real published-pack-to-new-pack installation
passed. Contact sheets were visually inspected. Neither Windows nor Proton
in-game visual acceptance of the additions has been performed; the published
pack is unchanged. Issue 19 was subsequently resumed; see the recoil candidate above.
The second sweep's independent check reconstructed all 251 new payloads from pinned DDS
artwork, explicit orientation and Xbox alpha with byte-exact results, and checked
both earlier packs for unchanged mappings and payloads; see
`second-sweep/independent-review.json`.

The third sweep accounts for every one of those 637 identities. It decodes
original texture objects from all 130 ENT/GEO/LST archives on the supplied PAL
PS2 disc: 6,265 objects, 3,216 distinct RGBA images. Named resources and nameless
geometry-bank objects are included. The audit reconstructs GS-block XXH3 texture
and palette hashes, cumulative mip chains and individual mip levels, normalizes
leading-zero hashes, and handles both indexed three-field and direct-color
two-field PCSX2 filenames. This resolves 2,337 of the pack's 2,435 DDS filenames
to original PS2 data. The remaining 98 have 80 palette-only links and 18 without
a source fingerprint; PAL/US differences mean this is not proof of absent art.
The PCSX2 reference tables and audit-only xxhash wheel stay private under
`third-sweep/vendor`; neither is shipped in the pack or linked into the runtime.

An exhaustive fine comparison tests all eligible byte-distinct DDS sources in
all eight orientations: 5,387,664 source/orientation comparisons. Only 162 of
the 637 groups pass the unchanged RGB/visible-alpha/resolution/aspect guards.
Visual review accepts 150. Twelve are deferred: differing face/hair, blood
colors, lighting colors, fire-frame patterns, tiny material ambiguity, and one
identified but dormant legacy shield archive outside the normal converter.
No threshold, alpha contract, runtime behavior or UI exclusion was relaxed.
Of the 150 additions, 149 have exact PS2 original texture AND palette hash proof;
the concrete texture `04-01\\map_04_conc1lr.tga` has one passing fine candidate,
palette agreement and visual pattern agreement, but its texture hash differs.
Do not describe that last pair as an exact original-source hash match.

There are now 487 unresolved supported gameplay identities: 407 diffuse/color
effects, 23 alpha-shape/near-uniform images, 33 tiny strips, 22 solid helpers,
one transparent helper and one dormant legacy candidate. The detailed audit
separates 169 close original counterparts whose HD sources fail fidelity/size,
43 named PS2 sources needing layout verification, 159 close originals without
an exact HD link, nine named originals without an HD link, 45 material helpers,
12 visually deferred candidates and 50 without a qualified counterpart. These
categories are review outcomes, not claims that every image needs an HD asset.
Five remaining groups share PS2 pixel indices with another palette; this does
not make a lit/off or differently colored material interchangeable. Ten arm
identities remain, including bloody variants missing from their apparent HD
counterparts. No guessed remapping was added for them.

Third-sweep scripts and evidence are in `build/hd-texture-coverage-audit/third-sweep`:
`decode_named_ps2.py`, `source_hash.py`, `find_via_disc.py`, `exhaustive_fine.py`,
`convert_verified.py`, `convert_final.py`, `package_verified.py`, `verify_final.py`,
`classify_all.py`, and `check_variant_links.py`. `remaining-before.csv` preserves
the exact 637-gameplay-identity input; do not rerun against the reduced report.
Approved contact sheets and source pins remain the conversion inputs; rerunning
the candidate ranking is not permission to overwrite the inspected choices.
The package hash is
`7f4e9481d20fb0f12a8fcb2bd9280ce984ce26961f7ec9d4285efcc1a1f6f8b4`
(207,635,772 bytes). `package-verification.json`, `independent-review.json`,
`source-fingerprint-summary.json`, `variant-summary.json` and `final-coverage.json`
record the checks. All 150 additions reconstruct byte-exactly; all 204 new source
pins validate, and all 340 earlier reviewed pins remain unchanged. Converter,
installer rejection/rollback and native world-texture fixtures pass, as does a
real Test-2-to-Test-3 installation in an isolated audit directory. The installer
test's duplicate-entry warning is intentional malformed-input coverage.

`releases/HD-Texture-637-Deep-Audit.csv` records every investigated identity and
source/proof status. `HD-Texture-Coverage-Comparison.csv` now compares all three
sweeps, and `HD-Texture-Coverage-Remaining.csv` contains 487 gameplay identities
plus the 35 previously excluded format/shared-UI records. The same optional
Test 3 ZIP and manifest serve Windows and Linux/Proton. No installed user assets
or published download were changed. Both platforms still require in-game
visual acceptance of these additions; offline source checks cannot establish it.

## Setup candidate

The October 30 setup candidate follows Darkwatch 0.7's source-selection flow:
Home offers Play/Select Game and Manage; Select Game offers direct ISO, import
once, and reuse of an existing installation. This does not add Darkwatch's
online updater. `installation.json` stores the selected GameData location;
neither Windows executable nor the Proton wrapper may force the default path.
Keep that record, GameData and directly used ISOs during package replacement.
Exactly one ISO/XISO under package `assets/` can prepare a first installation.
An existing or broken selection must never be silently replaced by discovery.

`prepare_xbe_release_assets.py` supplies the explicit `UCRR_GAME_ASSETS` /
optional `UCRR_GAME_ISO` runtime contract. Direct ISO mode verifies the disc and
retains only the Xbox executable and 27 ENT sources needed for Spanish and
co-op preparation. Original files remain in the unmodified ISO. The runtime's
derived `disc.cpp` supports the same five partition offsets as the importer.
Keep `prepare_xbe_release_assets` after the file/language preparers: Spanish
overrides must precede mounted-disc resolution, and the four generated co-op
LST additions must work through guest opens, host reads, metadata and directory
listing. All mounted DVD content remains read-only; save partitions stay writable.
Temporary originals required for optional PS2 voice conversion are removed when
conversion finishes or fails. The ISO remains required for direct-mode play.

Setup holds shared package/installation locks through mutations and gameplay.
The Windows UI must retain its busy guard through the final asynchronous status
and language refresh, otherwise an earlier operation can erase a newer cancel
token. Existing import repair retains `.previous-*` backups deliberately;
switching an already extracted install to direct ISO does not reclaim them.

Evidence is in `build/launcher-setup-20261030`: six linked runtime fixtures,
all 13 Python setup suites, 15 Bash wrapper tests, real Windows argv/UI worker
checks, and a real 190-file ISO verification passed. All four co-op derivatives
and 27 translated archives matched their expected hashes. The paired unpublished
0.08 Setup Test archives share 323 byte-identical files and contain no original
disc payloads or personal state. Rendered launcher previews were inspected.
A live launcher attempt hit a Computer Use approval timeout; direct-ISO gameplay
and live Proton acceptance remain unverified. Keep these distinct from the
successful offline runtime and real-disc setup checks.

The issues 17/18 candidate adds Pause > Cheats for keyboard/mouse and a Y
shortcut on the original controller pause screen. Main-menu Y is left to the
game so the original cheat-unlock sequence is no longer intercepted by Quit.
Only the ten codes present in this Xbox executable are exposed; ISEEYOU is
absent. Cheats retain the original shared profile semantics, including co-op,
and native weapon effects may require re-equipping or a mission restart.
Host clicks must pass through a copied state and a guest-thread update with
pause/profile identity checks; the UI must never mutate guest memory directly.

Controls has a separate Invert mouse look choice, default Off. Its saved
InvertMouseLook value only reverses the consumed vertical mouse-look delta.
Preserve independent stick inversion, mouse order gestures, pistol-spin input,
and Player 2's controller profile. Validate configuration rollback, actual
raw-input-to-camera flow and the real menu input fixture. Both paired packages
use the same code; offline tests do not establish live Proton compatibility.

The current candidate uses the pinned XboxXBERecomp runtime in
`build/upstream-review-xboxxbe-20261014`. The earlier `src/` runtime remains for
historical builds; it is not the current D3D12 executable.

The October 24 upstream review covers XboxXBERecomp through `e0de3e4` while
retaining the `c1f7d60` SDK pin. `prepare_xbe_missing_texture.py` backports
`c9cda4a`: a fixed-function operation reading an absent texture passes the
incoming colour, with programmable shaders and bound textures unchanged.
`prepare_xbe_audio_config.py` backports `5f760a2`: both the direct audio query
and full EEPROM report stereo consistently. Keep it after I/O preparation and
before language preparation, so language fixtures see the final kernel source.
Both fixes have production-derived regression tests, including original-behavior
negative witnesses; live Windows and Proton acceptance remains separate.

Upstream changes are adopted by active-path compatibility, not by replacing the
SDK wholesale. Preserve the packed-depth sampling exception, local audio routes,
generated title hooks, and existing input/import formats. The newer first-run
DLL compiler does not apply these title preparers. XboxXBERecomp is the sole
Xbox runtime dependency. The October 26 candidate replaces the remaining legacy
input/profile and ISO-import dependency with maintained title adapters. Controller
profiles consume XboxXBERecomp's input::Pad directly; the bounded ISO reader follows
its XDVDFS format contract. Existing save/binding formats remain unchanged.

Project-authored coroutine, prompt-art and shader-cache helpers now live under
scripts rather than being read from the retired runtime tree. Their staged bytes
are unchanged. Historical builds stay archived, but future source exports start
from the active source closure and must not inherit old runtime overlays. Root
CMake invokes the guarded current builder; it no longer builds the retired port.
Validate the dependency boundary along with the runtime and both package plans.
This dependency cleanup alone is not evidence of a frame-rate improvement.

The headroom candidate backports unchanged constant-upload reuse from upstream
`908dc53a` and redundant D3D12 binding suppression from `0fd00442`, through
`prepare_xbe_draw_bindings.py`. Keep it after the state-cost, draw-batch and
device-busy preparers. It must retain title constant extensions, command-list invalidation,
special-pass state changes and the existing descriptor/resource lifetime rules.
It does not adopt the unrelated upstream depth or GPU-pacing changes. Regression
checks establish rendering-state equivalence and reduced repeated work; live
Canals timing and image acceptance are separate evidence.

`prepare_xbe_file_query_efficiency.py` runs after language preparation, which
regenerates `files.cpp`. File-position and mode queries avoid unrelated flushes
and size lookups; metadata queries retain buffered-write visibility and the
existing Xbox status, output and completion contracts. Preserve this title's
mode response rather than copying a different runtime's semantics. Both paired
packages use these same runtime changes; Windows tests do not establish Proton
acceptance or an improvement on other hardware.

The lighting/particle CPU candidate keeps the recorded-index decoder's validated
output buffer at its high-water size. Warm draws use its existing validation and
decode passes without a separate caller count pass; growth still queries and
resizes, and malformed packets still trap before submission. No packet contents
are cached. The original XG compiler fixture now also invokes the maintained
caller, checking scratch reuse and copied index ownership.

The subsequent co-op CPU candidate uses the same full packet validator before
copying packed 16-bit indices directly into the retained renderer scratch.
Count-only validation can skip reading packed values when the permitted vertex
range is the complete 16-bit range; smaller bounds and odd 32-bit indices retain
their checks. This removes the intermediate 32-bit output and narrowing pass.
The original 32-bit and expanded-strip APIs remain available. Input stays immutable
and separate from output for each call, and the render queue owns its copy before
scratch reuse. Malformed packets and insufficient capacity publish no output.
The optimization uses the supported Windows/Proton x64 little-endian format;
it does not cache packet contents or change topology, degenerate indices, draws,
lighting or particle quality. Validate the original compiler, actual caller and
queued ownership together; isolated parser timings are not a gameplay FPS claim.

`prepare_xbe_inline_draws.py` bulk-copies validated non-incrementing INLINE_ARRAY
payloads during an active primitive. Other packets, visibility reports, state
changes and DrawUp ordering stay on the existing path. Its fixture executes the
prepared parser against the pre-optimization parser and original particle
producer. These changes reduce CPU submission work without changing lights,
effects, shaders or graphics quality; microbenchmarks are not gameplay FPS.
The proposed whole-range state-equality shortcut was rejected because changing
state regressed. The subsequent submission candidate preserves the existing
state deltas, but handles constant-sized 64-byte blocks before each range's
short tail. This lets the compiler specialize full copies without adding another
comparison, dirty flag or state cache. Queued bytes, field order, shader ownership
and delayed replay must remain identical. Explicit 12-byte and 20-byte resource
header copies preserve the original type selection, zero-filled tail and ring
snapshot lifetime; short headers must remain safe at a readable-page boundary.

The same candidate fills quad-list indices after one resize instead of inserting
six entries per quad. It preserves winding, index order, existing prefixes and
incomplete-quad discard. The original particle producer's 52-byte declaration
is included in the assembler fixture, alongside sparse, converted, unaligned
and exact-end index inputs. No geometry reuse or topology change is introduced.
Both changes are maintained by the existing state-cost and vertex-cost preparers;
their isolated CPU timings do not establish a Canals FPS gain.

The draw-reuse preparer also specializes the hash's four-byte remainder without
changing any hash result or seed. This retains existing shader, pipeline and
texture keys and the portable shader seed. Original and prepared hash fixtures
cover fixed vectors, mutation, length, alignment and readable-page boundaries.
The proposed last-pixel-key hash cache was rejected because misses regressed;
no new key cache or invalidation policy is introduced.

The October 29 CPU follow-up moves the converted index source choice outside
`ListIndices` through `prepare_xbe_vertex_costs.py`. Indexed ranged draws read
each authored 16-bit index with the same two-byte `memcpy` and subtract the lowest
index; the original base-vertex terms cancel under unsigned 32-bit arithmetic.
Other converted draws use their existing sequential order. Preserve topology,
winding, degenerates, list prefixes, incomplete primitives and raw/native paths.
The assembler fixture passed 11,482 cases, including wrapped bases, unaligned
and page-end reads. Its `dispatch` benchmark compares the immediately preceding
production assembler and measured roughly 10–25% less synthetic assembly time;
this is not a game FPS gain.

The next constant-upload candidate in `prepare_xbe_draw_bindings.py` assigns a
consumer-owned revision to replayed VSC bytes. Advance it for every copied state
interval overlapping the constants; never transport it from the producer.
An unchanged revision can skip comparison only for the same consumer state and
a still-valid GPU upload. Direct draws and uncertain ownership retain full byte
comparison, extensions retain fresh writable uploads, and revision overflow
permanently selects the comparison fallback. Keep upload invalidation at command
submission, frame reuse and settings reload, independently of content identity.
Final DrawConstants still compare their fully populated bytes.

The device-busy preparer creates `backend.h` from the SDK, so it must run before
the revision-aware Draw interface is installed. Verify the final header, replay
and renderer together after downstream preparation, including diagnostic call
sites. Evidence is retained in `build/performance-headroom-20261029`. The compiled
constant/replay fixture passed 1,554 delayed draws and 39 command-state snapshots;
its mock upload arena cannot establish real GPU fence/spill lifetime. Windows
single-player Canals rendering was observed, but repeated performance comparisons,
co-op acceptance and Linux/Proton acceptance remain pending. Both packages must
use the same adopted shared-runtime changes.

## Where to edit

| Area | Maintained source |
| --- | --- |
| Build and verification order | `scripts/build_xbe_game_trial.py` |
| Original function hooks and dependency overlays | `scripts/prepare_xbe_*.py` |
| PC menu drawing | `scripts/xbe_trial_graphics_ui.cpp`, `xbe_menu_style.h` |
| Menu state and original callbacks | `scripts/xbe_trial_guest_input.cpp`, `xbe_frontend_guest.inc` |
| Rebinding | `scripts/xbe_controls_*.inc`, `xbe_controls.h` |
| Body retention and character contacts | `scripts/prepare_xbe_ragdoll_settings.py`, `prepare_xbe_ragdoll_contacts.py` |
| Languages | `scripts/xbe_language.cpp`, `xbe_language_ui.inc` |
| Prompt classification and renderer handoff | `scripts/prepare_xbe_pc_prompts.py`, `prepare_xbe_prompt_handoff.py` |
| Optional HD UI art and original texture matching | `scripts/xbe_hd_ui.cpp`, `xbe_hd_ui_filter.h`, `prepare_xbe_hd_ui.py` |
| Field of view, widescreen HUD and reticle size | `scripts/xbe_view_layout.cpp`, `prepare_xbe_view_layout.py` |
| Original/HD host menu fonts and button atlas | `scripts/xbe_frontend_fonts.cpp` |
| Quiet release policy | `scripts/prepare_xbe_release.py` |
| Installed game admission | `scripts/prepare_xbe_release_assets.py` |
| ISO import, Setup and direct Play entry points | `launcher/burner-xbe/` |
| Experimental local co-op | `scripts/xbe_coop*.h/.cpp`, `prepare_xbe_coop*.py`, `coop_assets.py` |

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

### Experimental co-op qualification

The 0.08 candidate exposes experimental local co-op in the normal main menu,
through the same Play/Setup launcher and runtime as single-player. No production
`UCRR_COOP_TEST` flag or separate co-op executable is required. Availability alone
does not activate co-op: both Ready states, a connected controller and validated
campaign player admission are still required. Setup derives the character,
injection and paired-turret resources locally from the imported Xbox game; no
original model/animation archives are distributed. Offline fixtures cover input
routing, original actor admission boundaries, per-view ownership, character
resources and menu lifecycle. Live two-player startup, campaign behavior and
Windows/Proton acceptance remain separate requirements. The accepted native pause
action now selects the owning player's Controls and Graphics profiles. Ownership
stays fixed until that pause ends, while the whole game remains paused. Player 2
has independent bindings and native stick, crouch and vibration settings; the
original Player 1 profile remains intact. World and weapon FOV persist separately
for each player. Missing Player 2 FOV values migrate once from the existing Player
1 values, and later edits do not change the partner's view. Other graphics options
remain shared and the menu states this. Live input/menu and per-view acceptance
on Windows and Proton are still required.

Pause ownership follows the original frontend Start edge and accepted pause-state
entry, not the last device used. The owner remains fixed through its submenu
navigation; returning to gameplay or replacing the session clears it. Window
input is filtered before ImGui, and queued/held input is cleared at the owner
handoff. Player 2's pause uses controller prompts without changing Player 1's
underlying gameplay input mode. The native pause state still freezes both players.
Fixtures cover the original pause/input bodies, real ImGui handoffs, profile
save rollback and both players' camera projections. Candidate evidence lives in
`build/coop-player-settings-20261026/verification.json`.

Shared objective notifications use each player's original HUD queue. The native
publisher reads the last-created HUD singleton, so admitting two players alone
does not broadcast a mission update. Keep the repair limited to the ten verified
mission/special-objective call sites and their original argument contracts;
headshot, stun and evidence notifications retain their causal player ownership.
Shared scoring and objective state still execute once in the original caller.

Objective publication can precede the second camera and HUD. Use the requested
co-op session and validated HUD ownership for these messages, then deliver a
pending copy after the missing HUD's root is assigned. Do not weaken the active
session requirement of the personal-reward manager. Pending requests and silent
replica identities must not survive world or owner replacement. Both queues use
the existing per-view layout and native animation; only the mirrored request's
two original sound branches are suppressed, through its actual queue lifetime.
Offline queue/audio fixtures and paired packaging do not establish Windows or
Proton visual/audio acceptance.

Co-op HUD prompts capture the validated original layer owner at `2153EB`.
The quad sidecar retains that owner beside the text variant, and individual
draws publish both fields inside the renderer's transported state range.
P1 uses keyboard/mouse; P2 uses its own controller profile and the connected
controller's button family. Unowned menus retain automatic device selection.
The Orders behavior's keyboard override is restricted to its validated P1
owner; P2 retains the original controller gate. Keep original/HD atlas matching
and the separate dynamic prompt cache slots unchanged. Prompt fixtures cover
owner capture, reordered queue consumption, actual threaded transport and both
artwork paths; live Windows/Proton prompt acceptance remains pending.

The original OrderClient attaches to one OrderManager and stops advertising;
its HUD callbacks also read the last-created manager's singleton sprite list.
`prepare_xbe_coop_orders.py` gives each admitted player an independent native
command list and selects it within that player's HUD update/draw. Mirror native
command registration/removal without replaying NPC attachment initialization
(including medic stock). Eligibility reads use the checking player; only an
actual chosen command sets the client owner used by asynchronous completion.
Resolve live entity IDs on every use and preserve the single-player path.
Offline original-code fixtures do not establish two-player live acceptance.

Contextual help has a second singleton, `516140`, separate from the Orders
sprite list. Scope both HelpManager update and event publication to its validated
parent player: clearing an expired hint and adding a new one must reach the same
owned HUD queue. Shared NPC eligibility queries use that viewer's Orders manager;
the medic's original health query also needs that viewer instead of the campaign
player singleton. Keep command execution ownership and finite medic stock intact.
The native help tween ignores its incoming HUD rectangle, so its draw needs its
own per-view translation as well as correct queue ownership. Button artwork tags
alone cannot fix text sent to the wrong queue or drawn outside its owner's half.
Native selection fixtures cover separate look rays, looking away and occlusion;
queue, medic and layout fixtures cover the other stages. Physical two-player
gameplay and Proton acceptance remain separate checks.

Medic eligibility must use stored-pack capacity and the medic's finite stock,
not just the original injured-player health check. The transient HealOrderClient
is distinct from MedicOrderClient: its original yes/no/force-menu properties
unconditionally advertise an offer. Mirroring that client to both Orders managers
can therefore duplicate the Choose Heal wheel even with correctly owned HUD queues.
Scope offer eligibility to the validated recipient and that viewer's own target.
Only a real command may change delivery ownership; looking at a medic must not
redirect an in-flight handoff. Preserve the recipient's entity generation through
completion, and retain the existing receipt plus single-stock-decrement transaction.
Native patient healing, inventory limits and original stock initialization remain
separate contracts. Current video/source evidence and candidate qualification are
recorded under `build/medic-prompts-20261026`; live acceptance remains required.

First-person arm corrections validate original position, UV and skin streams
together after native skin loading. Bare, sleeved and Burner shoulder edges use
the original UpperArm influence instead of extending ForeTwist movement. Keep
hands, the short elbow ring and native animations unchanged, and preserve
texture density within each original atlas. This load-time correction does not
replace disc archives or require another HD texture download. Pose checks are
offline evidence; live ladder/shield appearance still needs qualification.

The private `BURNER_AIM_LOG` recorder compares actual shot/impact data with each
owner's rendered camera and reticle. It is compiled out of release builds,
has a hard capture budget and changes no aim or guest state. The private launcher
only enables it with `--aim-trace`; ordinary launches clear the inherited flag.
Treat these records as diagnosis, not proof that the reported hit offset is fixed.
The recorder also bounds firing-entry and rejected-shot coverage, and records
admitted actors, native input publication slots and render-layer ownership.
Reconcile those identities with the tester's input before assigning a capture to
a player. Compare the reticle's authored aiming texel rather than assuming its
geometric quad midpoint is the visible aiming point.
The temporal aiming experiment is deferred and is not in maintained gameplay.
CAB20 uses the previous target at `actor+460` for an unselected surface and the
current target at `actor+454` for a selected entity. A controlled C77D0/CAB20
regression demonstrates a turn-dependent difference, but that original behavior
also exists in single-player. The latest four live unselected-surface baseline
shots had equal current/previous targets and centered base rays; they do not
establish the history branch as the cause of the reported co-op offset. The
candidate source, fixture and hashes are retained privately under
`build/aim-correction-20261026/temporal-experiment`; its four maintained files were
restored exactly to the frozen baseline hashes. Earlier eight selected-entity
shots and the controlled proof remain under `build/medic-prompts-20261026/aim`.

The separate co-op reticle-alignment candidate centers the crosshair horizontally
within its player's viewport. Preserve the authored vertical placement,
single-player layout and all native trajectories, aim timing, spread and recoil.
The production-derived fixture covers 1,680 placement cases and rejects foreign
HUD owners, cropped or aliased sprite data and unsupported artwork. The private
Windows candidate loaded Canals and rendered both views during four upper-player
pistol shots; its capture is under
`build/aim-correction-20261026/candidate/sessions/20261027-005001-493013`.
This is a presentation correction. Sustained mouse movement, physical Player 2
controller firing and Proton gameplay still need acceptance; do not describe the
capture as a confirmed ballistic fix for the reported moving-shot problem.

The October 27 follow-up remains unresolved: the user still sees a right/down
offset. The verified private runtime captured 15 upper-player pistol shots in
`build/aim-followup-20261027/live/20261027-203007`. All shots have matched hit,
decal-publication and owner-camera records. The authored one-unit surface-normal
offset moves these marks only about half a display pixel down; CPU projection
does not explain the larger offset measured in the earlier video. This evidence
does not establish a root cause for a compensating shot-direction bias.
The packaged launcher repairs its managed runtime from `runtime/` using
`tools/runtime-manifest.json`; verify the executable hash after launch when
testing a private diagnostic, not just before starting the launcher.

The user subsequently requested a fixed three-display-pixel left calibration,
then confirmed it improved alignment but still fell right of center. The accepted
candidate uses nine display pixels total left (six more), based on the earlier
video's approximately 8.4-9.7 pixel horizontal offset at 2560x1440. This does not
establish exact centering for every weapon, view or resolution.
`TrialCoopShotOffset` applies it to each admitted co-op player's ordinary bullet
request immediately before CB3C8, after native spread. It uses that player's eye
basis, live camera FOV, viewport pixel aspect and the actual client presentation
width. Direction length and vertical projection are retained; impact marks still
come from the actual corrected collision. Single-player, other actors, electrical
beams and the separate projectile branch are excluded. This is an explicit
calibration, not a confirmed diagnosis of the original rendering discrepancy.
Offline checks cover both players, zoom/FOV, output widths, rotated views, native
spread and invalid identities. On October 27 the user confirmed "yep that fixed
the issue" after testing the nine-pixel Windows candidate (runtime SHA256
`017ca678338188fcc1a59cd54b1e24ac7b886026bf61b907142b41c87b507b81`).
Record this as user visual acceptance of the reported co-op aiming offset, not
an exhaustive weapon/resolution or independent physical Player 2 test. The paired
Proton package contains the same correction; live Proton gameplay remains untested.
Build and acceptance evidence: `build/aim-left-9px-20261027/verification.json`.

The first private gameplay test reached character selection but rendered a black
world. A dynamically created Mainplayer does not receive the campaign's authored
health initialization; the original update skips its camera coroutine at zero
health. Initialize only missing secondary health through the original setter,
retaining restored checkpoint health. A secondary layer can also exist before
its camera: skip that tracked unready layer, since the original null-camera path
clears the full screen. Regression fixtures execute the original startup gate
and view renderer; live two-camera acceptance is still required.

Independent player creation must use the original absolute factory `207CD0`.
The relative factory `207D20` with parent `FFFFFFFF` creates the registry entry
and view but cannot insert the actor into the parent-dependent world update
list. Do not work around this by manually advancing the camera coroutine.

The original audio update discards its camera when there are two views, placing
the listener at the distant null-camera fallback. Private co-op retains only
the validated live P1 camera supplied by the original view-zero caller. This
restores the shared spatial mix; it is not a separate listener for each player.
The Windows tester confirmed missing sounds returned; Proton audio is untested.

`coop_assets.py` derives a resource-only character archive from the supported
original multiplayer archive. Setup and Play validate the pinned source and
derived output; updates preserve installed saves, settings and learned caches.
The archive also includes the original spawn/death particles and their textures.
Readiness checks the parsed particle registry, because the original extension
callback can report success even when parsing failed. Preserve the reviewed ring
texture duplicates in source order: indexed and sequential lookups differ.
Do not distribute the derived original artwork. T-Zero uses campaign arms;
the four Burner bodies share the original Burner first-person arms.

The campaign distortion-target clear can reset a selected half-screen viewport
to the full target before scene drawing. The co-op hook at `9E761` restores the
original caller's viewport only for the validated two-player world and a matching
full-target reset. Keep the original SDK target-change behavior: the Xbox path
also resets its viewport, and effect compositors depend on it. The regression
executes the original effects round trip. The Windows tester confirmed both
views stayed separate during movement, gunfire and damage with candidate
`ce17b41b`; bounded traces confirm the lower-view repair executed. Full campaign
and Proton acceptance remain separate and pending.

Later heat-particle passes also switch targets after that early scene repair.
The five original billboard renderers and final heat compositor restore the
scene target but leave its viewport at the full target. The private effects
scope preserves each temporary target's original viewport and restores the
saved player viewport only on normal return to the same scene target/depth,
with the exact enclosing full-target reset. Keep the compositor's explicit
per-view XYRHW grid and the SDK SetRenderTarget contract unchanged. Its offline
regression covers both halves and the original target transitions. The Windows
tester confirmed "Both views look correct" in candidate `16ec1ec1` after the
fire/smoke comparison. This is scoped user visual acceptance; broader campaign
effects and Proton co-op still need separate checks.

The campaign actor's native body getter returns null. The private body overlay
resolves its tracked body at all three original matrix/crouch-update lookups.
Remove repeated hook text once before inserting any sites: removing it inside
the insertion loop silently discarded the first two hooks, leaving bodies at
their spawn transforms. Preparation verifies the hook count, and the regression
executes the original caller branch and setters for both moving owners. Native
skin animation and partner-body alignment still need live acceptance.

First-person body hiding must cover the original animated renderer, not only
static objects. Static `8A080/8A090` uses visibility byte `+50`; animated
`B5E80/B5E90` uses `+66`. The co-op bodies load the animated `.amf` path.
The entity hide flag alone is too late after the frame's visibility transfer;
scope the qualified renderer byte for each camera and restore its exact value.
Keep unknown interfaces unchanged and test actual animated draw admission.

Native action notifications need an additional qualified body call at `C1E74`:
the campaign player's null body getter and false network predicate otherwise
omit third-person fire/reload/throw animations. Forward the accepted action to
the tracked body's original `E57B0`, preserving first-person caller state.
Suppress duplicate body audio only inside that call. Do not replace the player's
network interfaces globally; other callers require their original null behavior.
The regression executes the original caller and sound boundary. The Windows
tester confirmed animations work in candidate `5aa92530`; complete action,
character and weapon-attachment coverage and Proton acceptance remain pending.

Third-person guns use different meshes from first-person weapons. Derive all
16 model roots named by original weapon tables `4FA090..4FA48F`, including their
material/texture dependencies, and require them during preload. A failed model
lookup can return success while leaving `ThirdPersonGun+74` null, which skips
its original hand-bone attachment update. Preserve that original attachment path.
The shared setup recipe upgrades the earlier character/effect-only derivative.

The generic secondary actor has no authored shield-unlock properties. Copy P1's
validated flag list into independently owned guest storage and retain its current
lock and borrowed activation-animation reference. For the two exact authored
training shield scripts, co-op admits each eligible actor to the original overlap
test and records its own pickup. Preserve the original scripts, props, sounds and
animation, except the command that disables the other shield pickup. Do not alias
the campaign's named PLAYER globally or infer shield availability from a mission
number. Restored checkpoints retain the original saved equipment flag.

Headshot accounting must preserve the original damage bit and medal path.
The retail projectile owner lookup recognizes only the player ID in the campaign
manager. Original player startup overwrites that ID, so P2 starting last reverses
which player the lookup accepts. Qualify both tracked local owners rather than
assuming the manager retains P1. The original headshot counter, deduplication and
eligibility remain unchanged; the killcam's killer comparison needs the same
two-owner treatment. Both players use the original pistol-spin detector, while
`xbe_coop_spin` keeps the earned reward with its validated actor. Route the
detector's grant/option-clear, reticle colour and killcam consumption through the
same owner; neither a partner's kill nor a partner's constructor may consume it.
The constructor owner at `CC404` is EBX; ESI is a string cursor. World/actor
replacement and session reset discard stale rewards. Single-player retains the
original global byte, and the enemy killcam's control/camera stack stays native.

The notification queue has its own last-created-HUD global. Capture each
validated player's native HUD tree at construction and select the responsible
player's queue at the original publisher's manager check. The user superseded
headshot fanout: headshot and incapacitation notices belong only to the actor
who caused them. Keep mission counters shared and incremented once. Preserve
causal ownership across aggregated damage and the delayed incapacitation award;
do not infer it from the latest input device or the last-created HUD. An expired
owner must not redirect a notice to the partner. Unrelated mission notices and
the single-player publication path retain their original behavior.

Evidence uses the original MaskPickup path, whose singleton player lookup can
miss the other co-op actor. Qualify either living local actor for the original
strict 150-unit sphere (player position plus 100 vertical units), then let the
native shared mask-bit update, sound and item retirement execute once. Downed
bodies do not claim evidence, and the collector owns its notification. No
global player alias or alternate score/save format is needed. Runtime fixtures
and live acceptance for this correction are recorded separately in
`build/coop-pickups-prompts-20261024`.

Ordinary weapons and ammunition follow collector-owned `C5040`: it grants into
the actor's +498 inventory and updates that actor's +7E8 HUD. Its source pickup
receives the collector ID and retires once. Keep the shared encountered-weapon
profile bit; it is not a grant to the partner. Co-op also rejects an already
retired pickup before a second collector can commit it during deferred removal.
Medic deliveries retain their existing recipient-owned count and finite stock.
The ordinary pickup regression joins the revive/spin paired candidate; live
pickup contention and ownership still require Windows and Proton testing.

Campaign rendezvous gates are derived from original PLAYER enter scripts, with
exact command/branch pins and root script fingerprints. The current census finds
43 direct or ScriptController gates across 17 campaign/bonus files. Gate only the
original successful enter/re-enter commit, using both living players' native
overlap. Leave startup/exit, hazards and pickups alone. Indirect flag-driven side
effects, modified archives and live route clearance remain separate checks.

Creating a DoorProp alone is scene population, not proof of a route-closing
operation. The earlier census incorrectly held Hope Street's garage population
at a thin entrance volume until both players overlapped it simultaneously,
delaying its truck, police car and shield-room door when players crossed apart.
Keep those population scripts native; retain rendezvous for explicit door
operations, barriers, teleports, cinematics and checkpoints. Door opcode 23 only
enables opening; it must not independently gate a scene. Opcode 43 disables
opening. Keep retirement events conservative: some also delete ladders or clear
earlier areas. The native opcode handlers and can-open predicate are pinned.
The regression replays every removed population/unlock root with both sequential
crossing orders, a legacy-census negative witness, and original single-player
controls. This verifies event admission, not a full mission playthrough.

The campaign audit reconciles all 17 supported maps with the original frontend
map tables, covering 524 PLAYER-enter groups. Eleven population-only and three
unlock-only roots retain native admission; 43 progression roots still rendezvous.
Startup/exit paths and unrelated trigger types remain unchanged. Structural
identity checks found no cross-map collisions among the 27 local level archives.
See `build/coop-campaign-audit-20261024` for the retained script/coverage evidence
and separate build versus live-acceptance status.

Ordinary NPC conversations retain one initiating actor through range/facing,
native input, lock, completion and abort. Both living players must satisfy the
authored finite range before admission and script handoff; negative remote/radio
ranges keep their original meaning. Only the initiator is held, and a mobile
partner leaving range delays progression without replaying speech. The global
ordinary-conversation fade is replaced by an owned skip/wait prompt. Authored
cutscenes still use shared fullscreen presentation. Cleanup must preserve both
the native control-lock flag and skip-ready byte without unlocking the partner.
When a partner leaves or falls before script handoff, release the initiator's
owned lock while retaining conversation ownership and cursor. This lets them
revive their partner and return together; completed dialogue must not replay.

Shared cinematic presentation admits only the pinned authored bone camera,
end-of-level camera and enemy kill camera. It renders the selected scene/HUD once
with the original fullscreen viewport, then restores the exact camera viewport.
Normal gameplay remains split. The host divider and health-pack HUD consume only
published state, never guest pointers. Cinematic listener selection remains one
shared original audio listener.

Generated co-op bodies and their owned attachments are hidden only during
qualified authored-camera render scopes, because those scenes provide their own
actors. Restore the exact static/animated visibility afterward. Enemy killcams,
player-death cameras and actual ragdolls are not authored-body suppression cases.

Stored health packs reuse the original stock-limited medic handoff and first-person
hypospray injection (`EQP_hypospray_Anim_Heal_Self.kfa`). Every fresh level starts
each player with one pack, with a carry limit of three. Original NPC-patient
healing is untouched. Counts are independent
and checkpointed alongside the same runtime `hdd` directory, keyed by profile,
level and complete original checkpoint bytes. Old checkpoints without this
companion restore zero packs rather than repeatedly granting a free pack.
Co-op deaths retain a native ragdoll during a ten-second revival window. The
downed player's view shows its remaining seconds; expiry uses the original
mission-failure path. Both players falling still fails immediately. Revival and
world/player replacement clear the deadline; no timer is saved in checkpoints.
The downed viewport has a pulsing red edge gradient with an untinted center.
The final three seconds pulse faster, including the countdown number. The
overlay clips to its owner's half and uses the same menu/cinematic gates and
revival reset as the countdown. Split-screen audio still has one shared listener
and stereo output; a master muffling filter cannot isolate the downed player.
Downed vision uses a queued nine-tap scene blur before the original HUD loop.
The guest boundary publishes a two-bit mask after both scene views finish. The
render thread takes one resolved scene copy and clamps all samples to the owned
half, including half-texel edges. It leaves the other view and subsequent native
and host HUD sharp. No blur command is queued outside admitted downed gameplay.
The living partner spends a pack only after a successful original player reset;
native body resources are recreated and retired through their normal lifecycle.
These additions require live gameplay acceptance on both Windows and Proton.

The first live partner-revival crash resolved to `FD89A`: an old gun attachment
could no longer find its retired body parent. Retiring arbitrary children during
the player's update also risks freeing the world's cached next list node. Queue
the old body for retirement at the boundary between the original world update
passes (`209258`), keep it registered until then, and retire its validated owned
attachments before the parent through native lifecycle calls. The original
retirement drain retains final destruction ownership. Tests need both the actual
parent-consumer failure and the update traversal ordering; a stubbed body reset
does not cover this lifetime contract. Exact crash evidence and candidate checks
are under `build/coop-revive-20261024`.

The live follow-up reported working gunfire but invisible first-person hands
and gun after revival. The original dead update `BDBC0` clears the two model
visibility gates at actor+2D0/+2D1; `D35A0` resets the actor without restoring
them. Restore the original startup gates once after that reset. `CCF80` still
applies normal weapon/shield availability and view visibility, so this does not
grant equipment or force hidden models visible every frame. Preserve inventory,
ammunition and the safe queued body retirement. Original dead/reset/render
fixtures and the paired candidate are recorded under
`build/coop-revive-spin-20261025`; live acceptance remains separate.

The later partner-injection crash resolved to the native animation event reader
`1F2CD0`. `kernel::CallGuest` returns guest EAX and restores the caller's general
registers; reading temporary EAX after that call supplied an unrelated actor
value as the clip pointer. Keep result consumers on the returned value, including
the medic-root, HUD-admission, co-op interaction and parsed-effect lookups.
Fixtures must retain this production adapter contract. The injection evidence
is under `build/coop-injection-crash-20261024`; live repeated revival remains a
separate acceptance check.

The private downed camera follows the original named `Bip01 Head` pose using a
pre-death eye-to-head calibration. It changes only the owned camera's pose,
preserving the existing FOV and other player's view. Reset cached calibration at
new-primary construction as well as owner/body changes, including address and ID
reuse. See [the camera contracts](COOP-DOWNED-CAMERA.md) for original paths and
offline coverage. Live eye alignment, clipping, revival and Proton acceptance
remain pending.

The private split HUD qualifies each tracked player's native root before changing
its corner inset. Keep the single-player layout, projected target brackets and
scope subtrees separate from this change. The native notification tweens discard
the descriptor and emit absolute coordinates, so their exact owned branch needs
an explicit lower-player offset as well as the upward move. Preserve relative
animation and fades; do not assume descriptor.top reaches the sprite queue.
The split scope's centered mask must temporarily receive its original horizontal
descriptor extent before suppressing the wider HUD transform, then restore the
caller descriptor exactly. Suppression alone shifts its center; retaining the
transform alone narrows its coverage. Single-player scope/tint behavior stays
separate. The fixture executes original notice tweens and scope centering through
their rectangle-queue boundary, rather than assuming already-offset quads.
The split divider uses a foreground draw list with an explicit full-viewport
clip; a regular menu window can clip its edges even when the quad spans the
screen. Offline draw-data checks cover width, seam coverage and visibility gates.

The stored health-pack icon is a small upright green square with a white plus,
and now follows the original owner's category-0 HUD fade during NPC dialogue.
The native fade query supplies icon/count opacity separately from health-pack
input admission, so a visual transition cannot cancel an injection. Test the
native query, both owners and partial fades alongside the production UI geometry.

The icon is drawn beside each health bar with square pixel proportions. It replaces the
earlier tilted model-referenced artwork, which is retained in `artwork/hud` as
source history but is no longer embedded in the font atlas. Each player's own
count and binding remain beside it. The guest update publishes the validated
native fill extent with the inventory snapshot so upgrades move the icon without
render-thread guest reads. No optional HD pack update is needed.
Live placement and readability on Windows and Proton remain separate checks.

Optional per-player Hold aiming is maintained in `prepare_xbe_aim_mode.py`
and `xbe_aim_mode.cpp`. Toggle remains the saved-profile default and original
query/cycle path. Hold reads the original remapped Zoom input as a held action,
uses the native zoom transition on a fresh press, and clears that request on
release. A separately assigned Cycle scope zoom input changes rifle magnification
while held; it starts unbound. Menu, focus, weapon and actor changes revoke a
stale hold. Keep raw input ownership, native query/transition fixtures, atomic
per-player profiles and the production Controls UI checks in the same build.

The aimed, nearby downed-partner prompt uses each player's binding and shows
“Revive Partner” with an independent short slide. Unbound controls show an
assignment hint. HUD/camera/pause gates, stock and readiness control visibility;
the prompt's 30-degree aim cone does not change the revive transaction itself.

Third-person shield presentation mirrors D6070's native weapon selector:
41F50's inventory gate and owner +24C choose shield ID0, and original +2AC
unavailable state chooses null. E55F0 then owns the idle pose and attachment
change; discrete actions still use E57B0. Preserve the ID25 mounted-gun exclusion,
whose nonnull definition lacks body animation metadata. Ordinary null definition
transitions are supported by the original function. See
`build/coop-shield-pose-20261024/verification.json` for the paired candidate and
offline checks. The Windows tester accepted the shield/body and health HUD
candidate in session `20261024-171441-527747`, then reported evidence pickup and
notification ownership problems. This is scoped user acceptance, not exhaustive
character/action coverage; Proton live acceptance remains pending.

### Paired Windows and Linux releases

Use Markdown (`.md`) for new README files and future release instructions.
The published 0.14 updater accepts `README.txt` and `assets/README.txt`, and its
Linux archive validator requires `README-linux.txt`. Renaming those package
entries without a compatible transition would break installed-launcher updates
before the new updater can run. Release 0.15 places the Markdown instructions
under the already-managed `tools/` directory and retains those legacy TXT paths
as compatibility pointers. Verify the transition using the actual published
updater on both platforms. Keep published archives and third-party documentation
unchanged.

The user requires Windows and Linux/Proton builds to stay current together.
Unless a request explicitly targets one platform, prepare both packages from
the same source revision, release version, verified runtime and shared manifests.
Build and validate the shared payload once, then assemble the Windows ZIP and
Linux/Proton archive with their respective launchers and instructions. Retain
the existing separate HD game texture download, compatible with both packages.

Check shared runtime, shader seed, restored HUD, bundled Spanish and optional
texture compatibility in both package inventories. Run the common importer,
state-preservation and package checks plus each platform's launcher checks.
Record Windows and Proton live acceptance separately, including tester feedback;
identify a pending platform check or blocker instead of silently shipping an
older package for that platform. Release notes should describe shared changes
once and identify platform-specific fixes and verification limits.

This is the standing preparation policy, not authorization to publish a new
release automatically. Keep previously published artifacts immutable during
testing and stage new candidates separately.

### Runtime build and packaging checks

Close the game first. Run the existing heavy-build wrapper with
`UCRR_XBE_RELEASE=1` and `scripts/build_xbe_game_trial.py`; clear that environment
setting afterward. This stages the overlays, builds the optimized executable,
and runs the production-contract fixtures. The build report and private symbols
remain beside the build, never in the release ZIP.

Use `launcher/burner-xbe/package.py` with the exact verified runtime SHA256 and a
fresh output directory, compile both launchers with its `build.ps1`, then run the
package audit and `test_setup.py --package`. Its explicit allowlist excludes game
assets, saves, learned caches and diagnostics. The hash-pinned portable shader
seed is the only cache exception. Never archive the workspace as a release.

The portable seed lives in `packaging/shader-seed`; regenerate it into a fresh
directory with `scripts/build_xbe_shader_seed.py --cache <observed d3d12 cache>`.
Multiple `--cache` inputs merge verified records. Run its Python fixture and the
compiled `test_shader_cache.exe --seed-directory <seed>` before packaging.
The package manifest pins every record; Play updates only `shader-seed` beside
the installed game EXE, leaving the player's `shader-cache` untouched.
Startup validates and warms exact source/entry/profile/flags/compiler identities.
A different compiler rebuilds the seed source locally before gameplay, and later
launches reuse that result. Never distribute `d3d12-pipelines` driver blobs.
The seed covers observed shader variants, not every possible draw. New variants,
driver pipeline creation and first-use texture uploads can still cause hitches.

Offline tests prove their stated contracts, not a full campaign playthrough.
Keep live menu, language/audio, gameplay and performance acceptance separate in
the candidate's verification notes.

### Optional Spanish voices

The paired packages also include the manual online updater described in
`docs/LAUNCHER-UPDATES.md`. Regenerate `package-manifest.json` after compiling the
launchers, audit it with the full package allowlist, and retain identical shared
manifests for Windows and Proton. Test archive validation, rollback and user-state
preservation on both platforms when changing the updater. Linux applies updates
through native Python to retain shell permissions; gameplay still uses Proton.

Spanish text remains in the main build. Optional Spanish voices are extracted
locally from the user's European multilingual PS2 ISO (SLES-53991) through the
same Windows/Proton Setup worker (`voices --iso`). Do not distribute a voice ZIP
or include original/converted recordings in either main package. The importer
pins the PS2 revision, original Xbox inputs and bundled Spanish-text sources,
checks all 64 generated files, and publishes `translations/es-voices` atomically.
Original imports, settings and saves are preserved. Existing voice installations
retain the same runtime manifest contract.

The runtime redirects only Spanish startup language reads to this set. Original
bank/event identities and stream ownership remain unchanged; movie video and
all audio tracks except the English voice slot retain their original bytes.
The main packages include the same converters and hash-pinned Windows FFmpeg
tools (also used through Proton), with their license and build/source notices.
`packaging/spanish-voice-tools.json` pins the tools staged from
`.tools/ffmpeg-8.1.2`; no converter is downloaded during installation.
The old delta pack builder remains private tooling, not a release deliverable.
Conversion source is the pinned PAL SLES-53991 disc; dialogue is matched by sound-program references,
not translated filenames. Intro 24/26 voices use a 2.916% pitch-preserving timing
adjustment to fit the Xbox movies. Offline codec/recipe checks do not establish
live audibility or lip synchronization on either platform.

Ragdoll contacts restore the earlier character/body admission change in the
original pair filter. Only its ragdoll-kind rejection is removed; property,
shape and collision-layer exclusions remain original. Default/More/Forever
still controls body retention independently. Do not globally disable collision
filters or change the physics solver to implement this behavior.

### Experimental paired helicopter turrets

The two campaign FirstPersonHelicopter instances use the same original event
path. The native player getter ignores its slot and returns the last registered
actor. Co-op routes the original mount/update/removal events to tracked owners,
retains separate native inventories and camera projection, and derives yaw
limits from each actor's own pitch. Do not replay the whole vehicle update or
its rotor/audio behavior. Turret records are reset with campaign player creation.

Only a verified co-op helicopter variant admits the paired mounting layout.
`coop_turret_assets.py` derives gun-free model aliases from the two pinned
original AMFs, retaining cabin geometry, bones, animation and collision support.
The shared installer builds `coopturrets03.lst` and `coopturrets08.lst` locally.
Neither original artwork nor a modified game archive is distributed. Keep these
archives separate: the native LST loader eagerly parses all models, so loading
the other campaign's alias would reference materials that are not yet mounted.
Load the matching archive at the exact helicopter model callback after campaign
resources are available; single-player and missing aliases retain original paths.
Reset alias admission at campaign preload, before entity construction.
The original collision companions contain only Chopper-Body-attached boxes
(ten in 3-01, four in 8-01); no separate removed-gun collider was found.
Preserve these cabin colliders rather than deleting collision with gun geometry.

The paired layout uses compact, uniformly scaled copies of the actual native
gun, preserving proportions. Its transform must precede native attachment and
muzzle queries; changing only rendered geometry would detach firing effects.
Partner views use the same animated gun with ordinary world projection/depth,
restoring its first-person flags afterward. ID25 has no original third-person
weapon metadata, so generated character bodies must never dispatch it through
the normal third-person weapon factory, including asset-fallback cases.

Static mesh clearance, native event fixtures and installer roundtrips do not
prove animated arm clearance or live firing. Both helicopter missions need
separate Windows and Proton acceptance, including dead/revived players and exit.

## Co-op character previews

The selector uses two rotating original 3D meshes, following Darkwatch's host-UI
approach. Each player's arrows and Ready button remain independent; keyboard
navigation owns Player 1 and the first controller owns Player 2. Mouse selection
can operate either panel, but continuing still requires both players ready and
the controller connected. Character changes and reopening reset preview rotation.
The original campaign browser and selected gameplay bodies/arms remain unchanged.

`xbe_coop_preview_assets.cpp` reads the existing locally derived
`coopcharacters.lst`; no additional artwork download or original-game payload is
distributed. Existing native parsers and skinning produce a static original
standing/lowered pose and the current co-op attachment mask. Four exact material
recipes are admitted, including visor/sunglasses/blade alpha surfaces. Wrapped
UVs are tiled into bounded images, not clamped. T-Zero uses representative texture
variation zero; gameplay can select its own original variation.

All five previews also hold the original third-person T-Zero pistol. FD530
selects `gundummy`; FD7F0 composes the 21B0D0 Y/Z basis conversion before that
posed bone and the body world. Keep that conversion: using the bone directly
turns the barrel through the character's face. The existing `coopcharacters.lst`
already contains the pistol and its texture, so no import/pack update is needed.
Its additional exact material recipe uses white texture alpha; all other recipe
admission, alpha surfaces and bounded atlas ownership remain unchanged. This is
cached selector geometry only, with no guest weapon or inventory mutation.

November 3 evidence is in `build/coop-preview-pistols-20261103`: all five bodies
with 2,422 additional pistol triangles each; upper-body attachment/orientation,
opaque pistol texture, malformed pistol/material/missing-socket rejection, and
original archive preservation. The production ImGui montage was visually checked
at front and 45 degrees for every character. This is offline visual evidence;
this candidate's live Windows and Proton acceptance remains separate.

`TrialGraphicsFonts` makes co-op available before input startup and reserves the
preview cells before the original font loader builds its atlas. Finalization
fills only those padded cells after font pixels are restored. Drawing rotates,
lights and sorts the cached mesh on the host UI thread using the existing ImGui
renderer, without entering the guest world or changing guest rendering state.
Average triangle-depth sorting is approximate for intersecting faces. Missing or
unsupported preview data shows a readable fallback while menu controls remain usable.

Offline checks cover all five original bodies, malformed resources, wrapped UVs,
source transparency, atlas ownership, font-pixel preservation, clipping, rotation,
large vertex offsets and independent input/readiness at three display sizes.
The original-model montage is generated from actual ImGui triangles and atlas
pixels. Windows and Proton live menu/gameplay acceptance remain separate; the
same runtime and existing asset recipe are staged for both.

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
`HUD-Help-57` (Choose Heal) uses the same mouse Orders gesture with horizontal
chevrons. The exact Context order hints `HUD-Help-48/102/106/107/108` use their
separate action12 binding, not the mouse gesture binding. These authored strings
use atlas rows14 and19, previously left as controller symbols even with correct
co-op ownership. Variant3 has its own GPU cache slot, separate from Orders,
interaction and controller artwork. Resource/producer evidence is retained under
`build/medic-glyphs-20261027/analysis`. Cover HD toggles, remapping, unbound keys,
both queued HUD owners and untouched menu/controller cells in the regression checks.
The tag travels with the renderer's copied state, so delayed draws cannot borrow
another prompt's binding. Each atlas variant has its own cached GPU resource.
HD artwork is applied before generated keycaps; it must not restore a replaced
controller symbol. Controller mode retains its original/remapped symbols.

## FMV framing

Graphics `fmv_scale` defaults to proportional Fill (0); Original aspect ratio
(1) preserves the complete encoded frame. Fill stretch (2) uses the verified
black-border bounds and stretches the remaining picture to the display. See the
November 5 Fill stretch section above for the current behavior and the November
4 audit for source provenance. No mode crops scene content. Only Fill stretch
changes proportions; unverified custom-film borders are not guessed per frame.

## November 6: native body shield and head/chest grabs

The accepted limb-drag response is retained. Head (native body 8) and chest
(body 4) now accept the same held Interact grab/release gesture. The compiled
input/physics fixture passes; live acceptance of the two new targets is pending.

The body-shield follow-up replaces bounded equipment springs with an independent
original-engine ragdoll held only at its chest. Its shared-world update runs once
per physics step, with separate ownership for both players. Native collision
exclusion applies only to the holder. Display and physics use original scale.
The generated recipe also fixes double-transformed visibility bounds responsible
for the reported flicker. See `docs/CORPSE-SHIELD.md` for verified seams, lifetime
hooks, asset derivation and the boundary between offline checks and live evidence.

Private receipts belong in `build/body-shield-native-20261106`. Pair Windows and
Linux/Proton packages from one checked runtime and source manifest; never include
original model archives, personal state or diagnostic logging in those packages.
This candidate is not a published release or a live Proton verification.

The following bash investigation is recorded in
`build/body-shield-bash-20261106`. A held actor-less native ragdoll could be the
first hit of the holder's original melee query: property `0x18` filters physical
contacts but not ray queries. The correction excludes only the current holder's
validated clone at the original ray predicate's accept path. Keep limb collision
IDs, world contacts and all original damage/push logic. See
`docs/CORPSE-SHIELD.md`; the asset recipe stays unchanged. Native filter and
ownership checks do not establish live bash acceptance or Proton compatibility.

## November 6 performance follow-up

The private performance candidate keeps visual settings and rendering operations
unchanged. `prepare_xbe_frame_costs.py` reuses each target's existing CPU pixel
snapshot in synchronous readback. Preserve the readback failure guarantee: the
output is not modified before resource creation and mapping succeed. Sync runs
on the render worker; its submission/wait path must not inspect that snapshot
reentrantly. `Changed`, CPU-overlay extraction, late-readback clearing, guest row
padding, copy ordering and source lifetime remain unchanged. The existing
readback target now also compares extracted old/new Sync bodies and consumers;
real GPU readback checks remain separate from CPU-only allocation benchmarks.

`prepare_xbe_draw_reuse.py` stores an owning compiled blob alongside the source
hash in the existing shader-source cache. Warm VS/PS hits avoid two extra blob-map
lookups per draw. Compile, shader keys, generated HLSL, pipeline identity and
portable shader-cache files are unchanged. The extracted shader-lookup fixture
covers success, cached failure, variants, shared blobs and map rehashing. One
extra reference is acquired only when a new source entry is stored.

The co-op orders `CopyCall` saves/restores its 4 KiB guest-stack region only if it
actually invokes guest code. Keep one snapshot for all calls in that scope,
exception-safe restoration, the recursion guard and original floating-point
state. Matching client lists need no snapshot. Other registration/event callers
invoke Run immediately and retain their existing behavior. The larger-list
regression from a proposed optional-wrapper version was rejected; the final
version keeps the original synchronization loop.

Focused measurements and exact input identities are in
`build/performance-20261106`. These CPU-path benchmarks are not gameplay FPS or
Steam Deck/Proton measurements. Use identical settings, warmed shaders, the same
scene and uncapped paired runs for gameplay comparisons. The active renderer
does not reuse geometry uploads: an earlier geometry cache was removed after
showing no useful reuse. Preserve immutable queued geometry snapshots.

## November 7: paired triangle-strip expansion

`prepare_xbe_vertex_costs.py` expands two adjacent converted strip triangles
together, reusing their shared edge. The complete index sequence, alternating
winding, degenerate triangles, base-vertex subtraction and incomplete-tail
handling remain byte-identical. The queued source stays immutable, and uploads,
shaders, lighting, effects, resolution and synchronization are unchanged.

The existing vertex fixture compares the extracted pre-change assembler with
the candidate, including short/odd/even strips, repeated indices, nonzero output
prefixes and 65,536-index input. `test_vertex_costs strip-pairs` is a synthetic
CPU benchmark only. Private implementation evidence lives in
`build/performance-20261107-geometry`; matched co-op Canals explosion evidence
belongs in `build/performance-20261107-coop`. Range/group render-state scans and
adjacent pixel-key caching were investigated but rejected after regressions;
they are not included in the runtime. Windows and Proton candidates must use
one runtime and source manifest; Windows results do not verify Proton hardware.

## November 7: vertex-constant submission candidate

`prepare_xbe_vertex_constant_costs.py` tracks the 48 existing 64-byte vertex
constant blocks in the audited producer state. Constant setters and viewport
updates mark their written blocks; shader-declaration loads mark the full range
once. Sending still compares and queues the original bytes, in the original
order. Untracked states, changed producer identity and reset state use the full
scan. Dirty metadata is not serialized and never substitutes for immutable
queued data or render-worker constant revisions.

Keep the writer-set assertions and extracted queue/replay fixture when changing
the D3D setters. A new writer must mark its range or disable tracking. The fixture
covers bitwise constants, clipped setters, shader declarations, resets, source
changes and delayed consumption; isolated cost tests include the setters. These
tests do not establish gameplay speed or visual acceptance. The current-runtime
comparison and rejected experiments are recorded in
`build/performance-next-20261107`, separately from the earlier release comparison.
Pair Windows and Proton packages from the same checked runtime; Proton remains
unverified until tested on Linux.

## Private blood-pool ledge candidate (November 12)

User accepted the No HUD scope fix. Blood-pool flag4 bypasses surface-intersection checks, not camera visibility; the earlier camera explanation was incorrect. The private ledge candidate caches supporting-ground polygons at creation through the original 214230 query and clips the growing pool with interpolated texture coordinates. Flat supported pools retain the original vertex path. Both 9BF50 and 9C018 entries are covered with byte pins and reversible preparation. Sampling is approximate; subcell holes require collision-triangle clipping.

Paired unpublished Windows/Proton v0.17 candidates, source snapshots, launch dependency validation and offline test evidence: build/reported-issues-20261112/ledge-candidates. Private runnable stage: ledge-live. Live ledge pixels, performance and Proton execution remain unverified. Audio unchanged and deferred.

The first ledge candidate regressed pool visibility: its half-unit support ray did not reach the floor beneath the original one-unit raised quad. The corrected private ledge-height candidate restores the original two-unit ray reach. A raised-quad/segment-floor regression fails before and passes after; Windows intro launch observed, but corrected pool pixels and Proton execution still require acceptance.
