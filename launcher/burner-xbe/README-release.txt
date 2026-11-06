Urban Chaos: Riot Response - Burner Edition 0.14

HD HUD is still a work in progress and has known prompt/display issues.
Turn HD UI Off to use the original HUD, which works normally.

PC PORT
Windows x64 PC port using XboxXBERecomp by TGP482, with native Direct3D12 rendering,
PC controls and menus. Original gameplay is statically recompiled from Xbox code.
XboxXBERecomp: https://github.com/TGP482/XboxXBERecomp

NEW IN 0.14 (SINCE 0.13)
- Added 16:10 and 1280x800, 1920x1200 and 2560x1600 resolution choices, with matching HUD, menus and cinematic layout.
- Added per-player left/right controller stick deadzones. Controller-only settings now stay on controller pages and do not affect mouse/keyboard input.
- Reorganized Graphics and Controls while preserving existing settings and bindings.
- Added body dragging: aim at a nearby dead enemy's limb, head or chest and hold Interact (F by default) to grab; hold again to release. Pulling is responsive and slightly lifts the grabbed part.
- Added an optional Burner body shield cheat in the main-menu Cheats list. The torso is supported while the head and limbs use ragdoll physics. Shield hits use body/blood effects instead of glass marks.
- With the cheat enabled before the first mission, both table shield pickups appear as static seated Burners. Collecting them retains the original shield progression and co-op pickup rules.
- Fixed body-shield visibility, its interception of the holder's attacks, and repeated model archive loading.
- Updated the locally generated shield assets automatically on Play. The separate 0.09 HD texture pack is unchanged; no reinstall is needed.

Co-op is experimental. Full-campaign progression, all character/action combinations,
helicopter sections and Proton co-op still need gameplay testing. The reported
co-op aiming offset was corrected and confirmed in the user's test.

INSTALL
Linux package users: follow README-linux.txt and its recommended Steam setup.
The steps below apply to the Windows package.
1. Extract the entire ZIP to a writable folder (not Program Files or inside ZIP).
2. For automatic first-time setup, put ONE supported Xbox ISO/XISO in assets/
   and open Burner Edition Setup.exe. It verifies the disc and prepares required
   files without copying the complete game. Keep that ISO for play and updates.
   An existing installation takes priority; dropping in an ISO does not replace it.
3. Alternatively, open Setup > SELECT GAME and choose one of these options:
   USE ISO DIRECTLY: select the image and keep it in place for future launches.
   IMPORT ONCE: copy and prepare the game locally; remove the ISO after success.
   USE EXISTING INSTALL: select a previous package or its GameData folder. Files,
   saves and add-ons stay there and are reused without being copied.
4. When ready, choose PLAY. Play Burner Edition.exe starts the same selected
   installation without opening Setup. Both single-player and co-op are included.

Both supported whole-disc ISO and XISO layouts are accepted. With multiple images
in assets/, use SELECT GAME to choose one. If an ISO used directly moves, select
its new location with USE ISO DIRECTLY. Setup never changes the source image.
Keep the EXEs, tools, runtime, assets and cover.png together. The default local
data folder is GameData; an external installation stays at its selected location.
No original disc payloads, Xbox executable, ISO, videos, saves or diagnostic logs
are included. Original textures, fonts, levels and audio come from your disc.

The ready home screen has matching PLAY and MANAGE buttons. MANAGE is grouped
into Settings, Game files, Add-ons, Updates and Support. Settings contains
Language and SKIP STARTUP. SKIP STARTUP defaults to OFF. Turn it ON to skip intro videos and
Press Start directly to profile selection on the next launch. Mission videos
are unchanged. The choice stays in this launcher's GameData/.skip-startup, even
when another installation is selected; saves and game settings stay untouched.
Close the game before setup or verification. If setup fails, read
the displayed message and last-setup-error.txt beside the launcher when present.
Spanish translation data is included in the main download. The first Play
after import prepares it from your game automatically; no separate Spanish
download is needed.
Only translation data is bundled, not complete translated game archives.
Single-player and Co-op use this same launcher and main menu. No separate co-op
executable or private launch flag is needed. First Play may take longer while
preparing character and animation resources locally from your selected game.
Approved restored HD UI PNGs are bundled in runtime/hd-ui. They replace only
matching original UI textures when HD UI is enabled; other UI art is prepared
locally from your game. Missing or invalid restored images keep the source art.
Keep the entire runtime folder, including hd-ui, with the launcher. The launcher
verifies the bundled files and asks you to re-extract the ZIP if it is damaged.
The bundled shader seed is installed automatically on Play, including when
updating an existing GameData folder. It prepares known shaders before gameplay
and keeps your learned shader cache. It contains no GPU-specific driver cache.
On a different Windows shader compiler, first startup may take longer while
the seed is compiled locally. Previously unseen shaders and other first-use
work can still cause pauses; the seed does not cover every scene in the game.

SETTINGS AND SAVES
Controls has separate Next Firearm and Previous Firearm keyboard/mouse bindings.
Next keeps your existing Firearms assignment. Previous defaults to Wheel Down
unless that input is already assigned in your saved profile; both are editable.
Movement inertia defaults to On for the original movement build-up. Turn it Off
in Controls for immediate directional input at the same speed. Each co-op player
has a separate saved choice; crouch, Walk and physical movement limits still apply.
Character collision with ragdoll bodies is restored. Ragdoll amount controls
body retention independently: Default, More or Forever.
Settings: GameData\xbe-runtime\settings.toml
Saves: GameData\xbe-runtime\hdd
Prepared game assets: GameData\installed\game_files
Shader cache: GameData\xbe-runtime\shader-cache
These paths are relative to the selected installation, which defaults to this
package. If GameData itself was selected, start directly at that folder.
Close the game and choose MANAGE > Updates > CHECK FOR UPDATES in Setup.
If a newer official Windows release is available, confirm its installation.
The launcher downloads and checks it, keeps a rollback copy, closes briefly and
reopens. Press PLAY to refresh the selected installation's managed runtime.
Saves, settings, installed add-ons, your source ISO and installation.json stay
in place. Update checks are manual; being offline does not prevent normal play.
If interrupted, reopen Setup to recover. Keep .launcher-update until recovery
finishes; it contains the rollback copy. Free disk space is needed for staging.
Older launchers need this complete package installed once to gain this feature.

For a manual update, close the game and Setup, then extract the COMPLETE new
package over the old one, replacing package files. Keep GameData, assets,
installation.json if present, and any externally selected installation folder.
Keep the ISO if using it directly. For a separate new package folder, choose
MANAGE > Game files > SELECT GAME / REPAIR > USE EXISTING INSTALL and select the
previous package or GameData. There is no need to copy or duplicate GameData.
Keep a backup of your saves before removing an old installation.
On the first ISO import, matching saves, settings and cache from the older
%LOCALAPPDATA%\UrbanChaosBurnerEdition installation are copied automatically.
The old installation is left untouched. Existing portable saves take priority.
Play uses the selected portable installation, never the older AppData installation.
Saves from the earlier pre-XBE port stay separate and are not migrated.
MANAGE > Game files > SAVES opens the selected installation's save folder.
Choose the original game language in Options > Language, then restart the game.
The supported disc supplies English, French, Italian and German. Spanish is not
present in this disc's text/voice banks, regardless of some ISO filenames.
The bundled community translation adds Spanish game text, PC settings and
control descriptions. Select Spanish in Setup > MANAGE > Settings > Language or in
Options > Language (see below).

LOCAL CO-OP AND HEALTH PACKS
Connect one controller, start normally, then choose Co-op on the main menu.
Player 1 uses keyboard/mouse and the upper view; Player 2 uses the controller and
the lower view. Select a character for each player, mark both Ready, then choose
a campaign level. This is local split-screen, not online multiplayer.
Opening either pause menu pauses both players. Controls and world/weapon FOV
edits apply to the player who paused; other graphics settings are shared.
Shared objectives appear in both views. Personal combat and interaction prompts
follow their player. Authored in-game cutscenes use one fullscreen view.

Options > Gameplay > Stored health packs enables or disables the system for
single-player and both co-op players. It defaults to On. Capacity follows
mission difficulty: Rookie 3, Veteran 2, Elite/Terror 1. Each fresh level starts
each player with one pack. Off restores direct medic healing, hides the pack
display and disables pack use and partner healing/reviving. With packs Off,
a co-op death ends the mission. Toggling the option never refills spent packs.
Get more from a medic while their original supplies last, including at full
health when your pack inventory has room. Look at the medic within range.
Assign Use health pack in Controls (keyboard default H). Keyboard/mouse also
has a separate Heal Partner control, default U unless that key is already used.
Controller users double-tap D-pad Down to heal themselves, or hold it while
facing a nearby injured/downed partner to heal/revive them. The giver spends
their own pack. Living partners have no healing prompt; downed partners retain
the Revive Partner prompt. These gestures affect only the controller's player.
With packs On, a downed player has ten seconds to be revived; expiry or both
players falling fails the mission.
The downed view pulses red and blurs; shared audio stays clear for the partner.
Stored packs also work in single-player. Existing checkpoints without stored-pack
data restore zero packs; they do not grant another free pack on every reload.

MOVIES
Graphics > FMV scaling has Fill (default) and Original aspect ratio modes.
Both preserve proportions. Fill removes only verified black rows already in
the original movie, then fits the complete active picture without cropping.
Original aspect ratio keeps the whole encoded frame, including its margins.
True 4:3 footage retains side bars on a widescreen display. Other remaining
bars depend on the source proportions; Fill does not stretch the image.
Existing saved choices remain respected.

NOTES
Switch 2 USB support is experimental: the native SDL USB driver and libusb DLL
are included, with Nintendo button prompts. Bluetooth is not implemented.
A data-capable USB cable and a Windows-compatible USB device driver are needed;
this package does not install or replace system drivers. Hardware testing is
still pending. Mouse users get clickable level and rewards browsers.

Requires Windows 10/11 x64 and a Direct3D12-capable GPU. The required Microsoft
Visual C++ runtime DLLs are included beside the game; no system installer runs.
Dynamic scenery lighting, character lighting, effects and retained ragdolls can
affect performance. Graphics options include lighting controls, MSAA, internal
resolution and ragdoll retention.
Normal gameplay writes no diagnostic logs or captures. A failed run can leave a
bounded last-error.txt beside the imported data to explain the failure.

OPTIONAL HD GAME TEXTURES
Download the separate Burner Edition HD texture pack ZIP. In Burner Edition
Setup, choose MANAGE > Add-ons > INSTALL HD TEXTURE PACK and select that ZIP (do not extract it).
Then start the game and enable Graphics > HD game textures > Apply.
The option is grayed out when the pack is not installed or is incomplete.
The pack stays in portable GameData and is separate from HD HUD and menus.
HD textures use more video memory. Original textures remain available with Off.
The separate 0.09 HD pack adds 565 texture mappings, for 1,803 total, on both
platforms. Install that ZIP to receive the new coverage; updating the game does
not automatically replace the optional pack. The older 0.06 pack still works.
The expanded coverage has offline checks but still needs in-game visual testing.
HD game texture artwork: xXthe.RockoXx; converted from the PS2 HD texture pack.

OPTIONAL SPANISH VOICES
Spanish text and PC-menu translations remain included in the main build.
Set up your Xbox game first and close the game. In Burner Edition Setup choose
MANAGE > Add-ons > IMPORT SPANISH VOICES and select your European multilingual PS2
Urban Chaos ISO (SLES-53991). If it is in a ZIP or RAR, extract the ISO first.
Setup verifies the disc and converts its Spanish dialogue and cutscene audio
locally. Allow at least 8.4 GiB of free space for conversion. This can take a
while; keep Setup open until it finishes. You can cancel
without replacing your previous voice installation. Then choose Spanish and Play.
No PS2 audio files or downloadable voice pack are included. The conversion tools
are bundled; FFmpeg does not need to be installed separately. Original game
files, other languages, saves and settings are kept. Previously installed voices
remain usable. Without imported voices, Spanish text uses English audio.
Windows and Proton use the same import process (see README-linux.txt).
The dub is from the European PS2 edition; ZARF's text translation is separate.
Two cutscene voice tracks are slightly accelerated without changing pitch to
fit the Xbox video timing. Clips that cannot be matched safely retain their
original recording. In-game audio and lip synchronization need testing.

KNOWN ISSUES
HD HUD still has prompt/display issues; the original HUD remains available.
Co-op is experimental, with incomplete campaign/character/turret gameplay coverage.
The Angel Heights officer placement fix was confirmed in a high-FPS Windows rescue test; full-campaign and Proton testing remain separate.
The targeted stone-ledge mapping repair still needs confirmation at the reporter's exact wall.
Spanish text and imported voice synchronization need community review.
A Linux tester confirmed setup and gameplay in the earlier compatibility build;
that does not verify this release's new features or co-op under Proton.

CREDITS
XboxXBERecomp runtime: TGP482. Urban Chaos adapters: Burner Edition project.
Additional dependency notices are in licenses/. This is an unofficial fan port.
Original game content belongs to its respective owners and must be supplied
locally by the user.

INTEGRATED COMMUNITY SPANISH TEXT
The ZARF beta translation and supplemental translations are included as text
data. On the first Play after import or upgrade, the launcher builds the
translated files locally from your selected game in
GameData/xbe-runtime/translations/es-zarf. English voices are retained unless
Spanish voices have been imported from your European PS2 ISO.
Choose Espanol (ZARF beta) in Options > Language, Apply, then restart. You can
also select Spanish in Burner Edition Setup > MANAGE > Settings > Language before PLAY.
The translation fills remaining English messages and includes Spanish PC menus
and control descriptions. Additional translations still need community review.

OPTIONAL DIAGNOSTIC LOGGING
Open Setup > Manage > Support > Diagnostic Logging before reproducing a problem, then
use Open Logs. Off by default; applies to the next game session. Files stay in
this launcher's GameData/logs even if you selected another installation.
Nothing is uploaded. Logs include setup errors/progress, runtime/system/device
details, settings, existing runtime warnings/reports, FPS samples and exit code.
They may include local paths and device names. Review before sharing.
Turn logging off afterward; it can affect timing. Runtime logs are capped at
64 MiB, setup logs at 8 MiB, with 30 files per category. Old logs are kept after
disabling. A full/read-only disk can prevent recording. No heavy GPU captures,
desktop typing or audio recordings are enabled.

