# Burner Edition: current runtime

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

Graphics `fmv_scale` defaults to Fill (0); 4:3 (1) preserves the original
proportions. Both now preserve the complete movie. The user requested this
change on November 1 after the earlier Black-style zoom cropped visible content.

`prepare_xbe_fmv_layout.py` pins the original open, close and movie-draw routines.
In the widescreen game viewport, Fill retains the full 640x480 movie quad and
unchanged UVs, stretching 4:3 footage to the output viewport without cropping.
4:3 narrows the picture horizontally and clears the side
bands after the movie draw, since movie playback skips the normal color clear.
Startup Dolby, Eidos and Rocksteady films obey the same Fill/4:3 selection as
campaign movies. Previously classifying these as Preserve forced 4:3 even in
Fill, producing a 960-pixel-wide image on a 1280x720 output. Unrecognized movie
names preserve their complete picture; menu background movies keep their
original layout. The authored alternate 448-line
draw and a 4:3 game viewport remain unchanged.

This does not detect or remove encoded black bars: borders baked into footage
remain. Fill retains titles and news tickers but changes 4:3 proportions; use
4:3 for the original shape. This does not add an ultrawide game viewport.

The draw fixture executes the original and patched guest routines and verifies
positions, UVs, clear ordering, guest state and movie lifecycle. Settings, menu,
language and skip fixtures cover integration. These are offline contracts;
Windows and Proton movie playback must each be checked before release.
