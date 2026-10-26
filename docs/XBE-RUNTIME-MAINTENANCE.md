# Burner Edition: current runtime

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
DLL compiler does not apply these title preparers. Some sp00nznet configuration
and ISO-import helpers remain active; its historical renderer is not used.

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
The aiming investigation is on hold at the user's request pending other gameplay
reports; no gameplay aim compensation has been applied.

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
drawn beside each health bar with square pixel proportions. It replaces the
earlier tilted model-referenced artwork, which is retained in `artwork/hud` as
source history but is no longer embedded in the font atlas. Each player's own
count and binding remain beside it. The guest update publishes the validated
native fill extent with the inventory snapshot so upgrades move the icon without
render-thread guest reads. No optional HD pack update is needed.
Live placement and readability on Windows and Proton remain separate checks.

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
The tag travels with the renderer's copied state, so delayed draws cannot borrow
another prompt's binding. Each atlas variant has its own cached GPU resource.
HD artwork is applied before generated keycaps; it must not restore a replaced
controller symbol. Controller mode retains its original/remapped symbols.

## FMV framing

Graphics `fmv_scale` defaults to Fill (0); 4:3 (1) preserves the complete movie.
The shared Windows/Proton runtime follows Black V2's proportional framing:
https://github.com/TGP482/BLACK-XBERecomp/blob/ac615d8d080cf5ca8789f805535b488267cb9728/src/game/hud.cpp

`prepare_xbe_fmv_layout.py` pins the original open, close and movie-draw routines.
In the widescreen game viewport, Fill expands movie geometry vertically about
its center by 4/3, leaving UVs unchanged. This crops the top and bottom without
stretching the picture. 4:3 narrows the picture horizontally and clears the side
bands after the movie draw, since movie playback skips the normal color clear.
Logos and unrecognized movie names preserve their complete picture; menu
background movies keep their original layout. The authored alternate 448-line
draw and a 4:3 game viewport remain unchanged.

This matches Black's framing rather than detecting black bars: bars baked into
wider cinema footage can remain, and Fill can crop titles or news tickers.
Use 4:3 to retain all source content. This does not add an ultrawide game viewport.

The draw fixture executes the original and patched guest routines and verifies
positions, UVs, clear ordering, guest state and movie lifecycle. Settings, menu,
language and skip fixtures cover integration. These are offline contracts;
Windows and Proton movie playback must each be checked before release.
