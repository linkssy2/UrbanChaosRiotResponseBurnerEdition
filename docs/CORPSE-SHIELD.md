# Burner body shield cheat

The main-menu **Burner body shield** cheat defaults Off and applies to
single-player and both co-op players. The normal shield must still be acquired.
Change the cheat before entering a mission. Original shield animation
registrations are not replaced during a running mission.

The November 6 native candidate replaces the rejected spring-driven equipment
motion with the game's original ragdoll: twelve bodies and eleven constraints.
Only the chest is keyframed to the animated shield support. Head, arms, pelvis
and legs remain dynamic, with native gravity, contacts and joint limits. There
is no looping sway, gait oscillator or per-frame reset of the free limbs. One
simulation per owner supplies both first-person and third-person presentation.
The original supporting arms and native shield raise/lower/bash animation remain.

The body is a private physics clone, not a new combatant or a corpse taken from
the level. It does not consume the corpse-retention pool. Native property `0x18`
excludes contacts with its holder, preserving contacts with the world and other
players. Display and physical body use the same original size. Shield protection,
recoil and bash damage retain their original gameplay path, with the holder's
own attack queries excluding the private held body as described below.

If native body creation fails, the full original shield loaders restore the
matching standard, upgraded or Burner shield, including animation bindings.

Blocked-hit sounds and particles use native flesh material 9 only for an owner
with a ready replacement; the original blood setting still applies. The five
shield-plane decal batches at MainPlayer `+2A4` are hidden by their pinned
`BC080` count callback for that owner. Stored glass damage is not erased.

## Assets and rendering

`corpse_shield_assets.py` derives recipe 4 `corpseshield.lst` from the player's
authenticated `shield.lst`, `40-01.ent` and `12-01.ent`. The archive includes the original
73-bone hidden physics model, fifteen presentation joints and required native
model/animation dependencies. UVs, materials and mesh topology remain original.
It is generated locally, never shipped as an original model archive. Extracted
and direct-ISO installs share the recipe and atomic replacement checks. Existing
saves and HD assets are preserved; no new HD texture download is needed.

The reported flicker exposed a separate visibility-bounds error. Original
`B8FE0` applies inverse-bind times composed-bone to each authored box; the old
recipe had already transformed the box, applying inverse-bind twice. In the
examined first-person pose 2,872 of 3,062 vertices lay outside the resulting
bounds. Recipe 3 stores boxes in the expected model space. Asset checks cover
all vertices, folded influences and raise/lower/bash/throw poses.

The real first-person loader path is `C8930 -> C8A74 -> BE050 -> 20BC40`
(return `BE075`, MainPlayer wrapper `+74`). ThirdPersonShield uses
`FE17F -> 20BC40` (return `FE184`). The earlier flashlight hook at `BD940`
was incorrect and is removed. Filename, caller, owner and resource identity
checks constrain substitutions to the shield. The flashlight at `+660` remains
independent.

### Table pickups

With the cheat enabled before entering the mission, the first-level table's
two shield props use a separate static, seated Burner model. It is derived from
the same Burner mesh as the held shield. The original pickup skeleton and
collision companions remain attached to the prop; the baked body mesh has no
independent ragdoll or character simulation. Collecting it still follows the
original shield scripts and the existing one-shield-per-player co-op rules.
The equipped shield continues to use the separate native ragdoll implementation.

The substitution is restricted to the authored table props and their original
shield model load. Cheat Off leaves the normal table shields in place. Launcher
asset validation regenerates the local derived archive when its recipe hash
changes, for extracted and direct-ISO installations alike. No new original game
archive or HD texture pack is distributed.

## Physics and lifetime

The hidden 73-bone wrapper supplies the original skeleton interface. Constructor
`2A6130` clones the native ragdoll; chest body 4 maps to original Spine2 bone 5.
The chest is converted to native motion type 6 before insertion. `2A5F00`
activates the clone, and `2A5DA0` seeds its initial pose once.

Around the original shared `29E5B0 -> 2ED370` world step, `2E84A0` supplies
centre-of-mass-aware chest linear/angular velocity using inverse delta time.
After the solve, `2A5DA0` publishes the native skeleton to the render aliases.
The hook does not run a second world solve or advance physics once per viewport.
Both owners have separate models, physics bodies, target transforms and poses.
The target reads the current MainPlayer `+6F0` world matrix, produced by `CE670`
before the shared physics step, rather than the later renderer's cached copy.

Release runs at `208D90` before entity/resource destruction and at `29E930`
before physics-world replacement. The native clone is destroyed before its
hidden model wrapper. Both entry hooks are needed and release is idempotent.
Switching equipment or losing a valid owner also retires the private clone.

## November 6 bash follow-up

The user accepted the native body presentation but reported that bashes did not
push enemies. The supplied clip shows repeated body-shield thrusts at a nearby
enemy; occlusion prevents measuring each hit from the video alone.

The original `D0E20` MeleeHit event still enters `C57C0`. Its `214230` query
stops at the first hit, and an actor-less hit cannot receive enemy damage or
push. The private ragdoll is actor-less and keeps unique native limb collision
IDs. Contact exclusion property `0x18` does not affect the original `2A8860`
ray predicate, so the held body could intercept its holder's bash.

The correction rejects only that holder's current, validated clone in the ray
predicate's accept path. The query already carries the holder's excluded group:
`C57C0` passes owner `+94` low16 to `214230`, and `2A7470` packs it into ray
input `+24` low20. Exact limb identity and current owner/world checks prevent
this rule from affecting ordinary corpses or another player's held body.
All original filter rejections, physical contacts, joint constraints and enemy
attack queries remain intact. Do not assign the holder ID to every limb: that
would also remove the original limb self contacts.

Follow-up receipts belong in `build/body-shield-bash-20261106`. The regression
uses the original byte-pinned ray predicate and separately tests the actual
held-body registry. Scene traversal and live enemy reactions remain separate
acceptance checks. No asset recipe or HD pack change is required.

### Bash lethality

The follow-up report of dead enemies and a `Brutal` notice is consistent with
the original lethal shield attack; it is not a guaranteed nonlethal knockout.
Original `54FE0` initializes item 0 to 150 or 300 damage according to progression.
The read-only snapshot `damage-state-20261106-121815.json` in the bash evidence
folder found 300 damage, source 8, and no enabled retail cheats in the running
candidate. Raw executable data is uninitialized here and must not be used as
the live damage value.

`E0470` routes source 8 to `E03A0`; item 0 adds flag `0x400000`. The original
health/scoring path awards `HUD-Headshot-2` (`Brutal`) on an enemy's transition
to dead. The held clone has no actor or impact receiver, so its native contact
callback does not add ragdoll impact damage. The bash correction does not alter
damage, progression or this scoring path. No damage reduction was made.

The snapshot was taken after combat, not synchronized to a particular hit.
It does not establish each victim's pre-hit health or a matched normal-shield
comparison; indirect displacement into hazards remains outside this check.

## Verification

Private evidence belongs in `build/body-shield-native-20261106`. Checks cover
pinned original functions, native hard-keyframe math, loader/pose boundaries,
shared-step CPU/memory parity, holder-only collision exclusion, lifecycle,
original/display size parity, skinning and culling bounds. Contract mocks are
identified in fixtures; they do not establish a complete live physics solve.

Windows and Linux/Proton use the same runtime and asset manifest. The user has
accepted the native body-shield presentation; the subsequent bash correction
still needs in-game enemy-reaction acceptance. Live Proton is untested.
