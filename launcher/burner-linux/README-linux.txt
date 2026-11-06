Urban Chaos: Riot Response - Burner Edition 0.14
EXPERIMENTAL LINUX / PROTON PACKAGE

This runs the same Windows x64 game through Proton; this is not a native port.
Windows and Linux/Proton use the same runtime, installation recipes and optional
HD texture pack. These setup changes have automated checks; live Linux testing
of this build remains pending.

QUICK START THROUGH STEAM (RECOMMENDED)
You need Linux x86-64, Steam, an installed Proton version, working Vulkan drivers
and your supported original Xbox disc image. On Steam Deck, use Desktop Mode.

1. Extract the complete Linux package into a writable folder. Keep tools,
   runtime, assets and licenses together. Do not run from inside the archive.
2. For automatic first-time setup, put ONE supported Xbox ISO/XISO in assets/.
   Keep the ISO in that location: this mode reads original files from the disc
   image during play. Setup still verifies it and prepares the required files.
3. Add tools/python/python.exe as a Non-Steam Game. Show all files if necessary.
   Name the shortcut Urban Chaos - Burner Edition. Set its Start In field to
   the extracted package folder, enclosed in double quotes.
4. In Properties > Compatibility, enable the compatibility-tool override and
   choose your installed Proton version. Experimental is optional, not required.
5. Set Launch Options to:

   -B "tools/import_game.py" launch --interactive

6. Press Play. A first-time preparation message appears; acknowledge it and
   leave Steam running. The game starts after successful preparation. Errors
   are shown in a dialog and saved when possible. Keep the same shortcut and
   Launch Options for later sessions.

An existing installation takes priority over assets/. Dropping in an ISO does
not replace it. If there is more than one image in assets/, choose explicitly
using the commands below. Both supported whole-disc and XISO layouts work;
renaming an unsupported image does not make it compatible.

SELECT GAME: THREE SETUP CHOICES
Temporarily replace Launch Options with ONE command below, press Play and wait
for completion. Then restore the normal launch command from step 5.
Paths are relative to the package folder unless an absolute path is supplied.
Always put paths containing spaces in double quotes.

Use ISO directly:
  -B "tools/import_game.py" use-iso --iso "assets/game.iso" --cancel ".cancel-use-iso" --interactive

The ISO stays required for play and future preparation. If it moves, repeat this
command with its new location. Your source ISO is never modified.

Import once:
  -B "tools/import_game.py" import --iso "assets/game.iso" --cancel ".cancel-import" --interactive

Copies the original game files into the installation. You may remove the ISO
after successful setup. Existing saves, settings and compatible add-ons remain.

Use Existing Install:
  -B "tools/import_game.py" use-installation --installation "../Previous Burner Edition" --interactive

Select the old package folder or its GameData folder. Its files, saves and
add-ons stay there and are reused without copying. Keep that folder available.
The selected installation is shared by subsequent play, language, texture and
voice operations. Do not manually add --data unless intentionally overriding
that selection for one command.

OPTIONAL STARTUP SKIP
Skip Startup defaults to OFF. To skip intro videos and Press Start directly to
profile selection, temporarily set the same Steam shortcut's Launch Options to:
  -B "tools/import_game.py" startup --skip-startup on --interactive

Press Play and acknowledge the message, then restore the normal launch command.
Use off instead of on to restore the original startup. Mission videos are
unchanged. The choice is saved in this launcher's GameData/.skip-startup, even
when another installation is selected. Saves and game settings stay untouched.
The terminal alternative is ./burner.sh startup on (or off); startup with no
argument shows the saved choice. The Windows Setup menu through Proton has the
same SKIP STARTUP toggle under MANAGE > Settings. Its grouped Settings, Game files,
Add-ons, Updates and Support sections are shared with the Windows launcher;
the Steam commands and terminal alternative below remain available.

OPTIONAL DIAGNOSTIC LOGGING
Logging is off by default. Before reproducing a problem, temporarily set the
same Steam shortcut's Launch Options to:
  -B "tools/import_game.py" diagnostics --logging on --interactive

Press Play and acknowledge the message, then restore the normal launch command.
Use off instead of on to disable it. Files stay in this launcher's GameData/logs,
including when another installation is selected. Share relevant logs manually;
nothing is uploaded. They may include local paths, device names and settings.
The terminal alternative is ./burner.sh diagnostics on (or off).
Game logs are capped at 64 MiB; setup/terminal Proton logs at 8 MiB each, retaining
30 files per category. Turn logging off afterward; logging can affect timing.
It does not enable heavy GPU captures, input/audio recordings or memory dumps.
Steam/Proton failures before the Python worker starts cannot be logged by it.
The advanced wrapper additionally records output from its Proton worker launch.
A read-only/full disk can prevent logging. Existing logs remain after disabling.

VERIFY OR REPAIR
To verify the selected installation without starting the game:
  -B "tools/import_game.py" verify --interactive

For a missing or moved direct-mode ISO, choose its new location with use-iso.
For a damaged extracted installation, repeat import with the supported ISO.
A broken installation is not silently replaced by a different image in assets/.

OPTIONAL HD GAME TEXTURES
Download the separate compatible HD texture ZIP; do not extract it. Temporarily
use, for example:
  -B "tools/import_game.py" textures --pack "HD-Game-Textures.zip" --cancel ".cancel-textures" --interactive

Restore the normal launch command after success, then enable Graphics > HD game
textures > Apply. The separate 0.09 HD pack adds 565 mappings, for 1,803 total.
Install the new ZIP to receive those additions; updating the game alone keeps
your existing pack. The 0.06 pack remains compatible. HD HUD artwork stays in
the main package. Expanded texture coverage still needs in-game visual testing.

LANGUAGE AND OPTIONAL SPANISH VOICES
Bundled Spanish text is included; no translation download is needed. Set language
in the game and restart, or temporarily run this command before playing:
  -B "tools/import_game.py" language --language 4 --interactive

Language values: 0 English, 1 French, 2 Italian, 3 German, 4 Spanish. Changing
language preserves other settings. Restore the normal launch command afterward.

Optional Spanish voices come from YOUR European multilingual PS2 ISO,
SLES-53991 (En/Fr/De/Es/It). Set up the Xbox game first. Extract the PS2 ISO from
its ZIP if necessary, then temporarily use:
  -B "tools/import_game.py" voices --iso "ps2-europe.iso" --cancel ".cancel-voices" --interactive

The bundled converters prepare dialogue and matched movie audio locally. Wait
for completion, select Spanish and restore the normal launch command. You may
remove the PS2 ISO after successful voice import. Original Xbox files, other
languages and saves stay intact. No original recordings are included here.

SINGLE-PLAYER AND CO-OP
Use the same play command for both modes; choose Single player or Co-op from
the main menu. Co-op uses keyboard/mouse for Player 1 and a controller for
Player 2. Both players pause together and edit their own Controls and FOV.
Controller users double-tap D-pad Down to heal themselves, or hold it while
facing a nearby injured/downed partner to heal or revive them. Keyboard/mouse
uses Use health pack for self-healing and the separate Heal Partner binding
(default U unless already assigned). Treatment spends the giver's pack.
Options > Gameplay > Stored health packs applies to both players and single-player.
On is the default: capacity is Rookie 3, Veteran 2, Elite/Terror 1, with one pack
per player at a fresh mission start. Off restores direct medic healing and
disables stored packs, partner healing and reviving. Co-op death ends the
mission with packs Off; On keeps the ten-second revival window.
FMV Fill defaults to proportional fitting with verified source margins removed;
true 4:3 footage retains side bars. See README.txt for details.
Live Proton co-op acceptance for this build remains pending.

UPDATES AND SAVES
From 0.09 onward, close the game and stop the Steam setup/play shortcut. Open
a terminal IN the extracted package folder, then run:
  ./burner.sh check-updates
  ./burner.sh update

Updates require native Python 3.10 or newer (separate from bundled Windows
Python). No Proton environment or new Steam shortcut is needed for these two
commands. The updater downloads the official Linux package, verifies it, keeps
a rollback copy and replaces only managed package files, preserving executable
permissions. When it prints success, press Play using your SAME Steam shortcut.
Play refreshes the selected installation's managed runtime and bundled content.
Saves, settings, installed add-ons, ISO, installation.json and prefixes stay put.

If interrupted, run ./burner.sh recover-update before playing again. Keep the
.launcher-update folder until recovery finishes. Do not update while the game
or Steam setup worker is open. Linux updates that change the bundled Windows
Python require a manual complete-package update, to keep Steam recovery safe.
Older launchers need this complete package installed once to gain the updater.

For an in-place update, close the game and extract the COMPLETE new package
over the existing package, replacing package files. Keep GameData/, assets/,
installation.json if present, and any externally selected installation folder.
Do not delete an ISO used directly. Keep the same Steam shortcut.

If using a new package folder instead, choose Use Existing Install and point to
the old folder. There is no need to duplicate its GameData. Play refreshes the
managed runtime and bundled content without replacing saves or settings.

Default locations (inside the selected installation):
  Saves: GameData/xbe-runtime/hdd
  Settings: GameData/xbe-runtime/settings.toml
  Prepared/original content: GameData/installed/game_files
If GameData itself was selected, these paths start directly at that folder.

Never delete GameData to troubleshoot Proton. Steam manages the prefix for its
shortcut. The optional terminal wrapper uses a separate .proton folder. Keep
managed files/folders as real files rather than symlinks; ordinary host path
mapping used by Proton is supported.
Keep the same Steam shortcut when updating; if the package moves, edit its Target
and Start In together. Recreating the shortcut can select a different prefix.
The terminal wrapper resolves a symlink to burner.sh back to its real package.
Do not replace package files while its embedded Python or the game is running.

IF SETUP OR PLAY FAILS
Steam/Flatpak must have access to the package, the selected installation and any
ISO used directly. Our package does not install Proton, Wine or graphics drivers.
The recommended route uses the bundled worker; the Windows .NET Setup window is
not needed under Proton.

Setup errors appear in a dialog and, when writable, last-setup-error.txt beside
tools and runtime. Game failures may also write last-error.txt inside the
selected GameData folder. Include the exact error, version, Linux distribution,
GPU/driver, Proton version and failing step in a report.

If no dialog appears, or video is black while audio/FPS remain, enable a Proton
log temporarily:
  PROTON_LOG=1 %command% -B "tools/import_game.py" launch --interactive

Close the game after reproducing the issue and look for the newest steam-*.log
in your home folder, or PROTON_LOG_DIR if you set one. For setup errors, put
PROTON_LOG=1 %command% before that setup command's -B arguments instead.
Restore the normal command afterward. Logging is optional troubleshooting,
not a required setting or rendering fix. Explicit environment options are kept.

OPTIONAL ADVANCED TERMINAL WRAPPER
The wrapper invokes Proton directly; it does not configure the Steam Linux
Runtime container. Prefer the Steam route above if host libraries cause errors.
From the extracted package folder:

  bash burner.sh play
  bash burner.sh use-iso "/path/to/game.iso"
  bash burner.sh import "/path/to/game.iso"
  bash burner.sh use-installation "/path/to/previous package or GameData"
  bash burner.sh verify
  bash burner.sh auto-setup
  bash burner.sh textures "/path/to/HD-Game-Textures.zip"
  bash burner.sh voices "/path/to/European PS2.iso"
  bash burner.sh language spanish
  bash burner.sh startup on
  bash burner.sh status
  bash burner.sh --help

Play has the same first-run assets/ discovery as Steam. Status does not start
the game: exit 0 means ready, 2 means not ready. Language names are english,
french, italian, german and spanish. With no language argument it shows the
current selection.

Choose a Proton version explicitly if needed:
  BURNER_PROTON="/path/to/Chosen Proton/proton" bash burner.sh play
For a nonstandard Steam folder, also set BURNER_STEAM="/path/to/Steam".
The wrapper uses Experimental if installed; otherwise it accepts a sole Proton
in the main Steam library. With several choices, set BURNER_PROTON explicitly.
Explicit Proton and driver environment options are preserved.

VERIFICATION LIMITS
Automated tests cover command routing, file/path quoting, installed-Proton
selection, preserved environment/state and failure handling. They do not prove
Linux video, audio, frame pacing, FMVs, controllers or Steam Deck compatibility.
Earlier user feedback confirmed a working Linux setup/gameplay session; it does
not replace testing this build. The shader seed is portable shader data, not a
Linux GPU driver cache.

Release details and credits: README.txt and licenses/
Proton: https://github.com/ValveSoftware/Proton#runtime-config-options
Steam Linux Runtime: https://github.com/ValveSoftware/steam-runtime
Direct3D 12 translation: https://github.com/HansKristian-Work/vkd3d-proton
