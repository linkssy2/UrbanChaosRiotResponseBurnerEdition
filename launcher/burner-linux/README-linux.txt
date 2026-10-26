Urban Chaos: Riot Response - Burner Edition 0.08
EXPERIMENTAL LINUX / PROTON PACKAGE

This runs the same Windows x64 game through Proton; this is not a native port.
A Linux tester confirmed setup and gameplay with the earlier compatibility
build. That does not verify 0.08's new features, co-op, every GPU or Steam Deck.
The Windows and Proton packages share the same game runtime and asset recipes.

SET UP THROUGH STEAM (RECOMMENDED)
Use Steam for both setup and play so it supplies Proton's Linux runtime.
You need Linux x86-64, Steam with Proton Experimental installed, working Vulkan
drivers for your GPU, and your supported original Xbox ISO or portable GameData.
On Steam Deck, do the setup in Desktop Mode.

1. Extract the complete Linux archive to a writable folder. Keep tools, runtime
   and licenses together. Do not run from inside the archive.
2. If you already imported the game, COPY your GameData folder into this folder
   and skip step 5. Keep a backup of the original.
   Otherwise, place your ISO in this folder and name it game.iso. Both supported
   full-disc ISO and extracted XISO layouts are accepted; renaming does not
   change the disc layout or make an unsupported disc compatible.
3. In Steam, choose Games > Add a Non-Steam Game > Browse. Select the extracted
   tools/python/python.exe (show all files if necessary), then add it. Rename
   the shortcut to Urban Chaos - Burner Edition.
4. Open that shortcut's Properties. Set Start In to the extracted package folder
   containing tools and runtime, enclosed in double quotes. Under Compatibility,
   enable "Force the use of a specific Steam Play compatibility tool" and choose
   Proton Experimental. Keep this same shortcut for setup and play.
5. For the first import, set Launch Options to this exact line:

   -B "tools/import_game.py" import --iso "game.iso" --cancel ".cancel-import" --interactive

   Press Play. A message confirms that import is about to start; press OK and
   wait for the completion or error dialog. Import can take several minutes;
   there is no progress window. A successful import also creates
   GameData/installed/installed.json. If no dialog appears or import fails,
   see IF SETUP OR PLAY FAILS below.
6. Set Launch Options to the normal play command:

   -B "tools/import_game.py" launch --interactive

   Press Play. Keep these launch options for future runs. The first launch can
   take longer while it prepares game files and known shaders. The ISO is no
   longer required after a successful import.

Steam/Flatpak must have access to the extracted folder and its files. This path
uses the bundled Python worker; the Windows .NET setup window is not required.
Our package does not install Proton, Wine or GPU drivers. Keep launching through
the worker above so runtime updates and bundled translations are applied.

SINGLE-PLAYER AND EXPERIMENTAL CO-OP
Keep the normal play command from step 6 for either mode. Choose Single player
or Co-op in the game's main menu; no separate co-op launcher or launch flag is
needed. Co-op currently uses keyboard/mouse for Player 1 (upper view) and one
controller for Player 2 (lower view). Select characters, mark both Ready, then
choose a campaign level. Both players pause together; the player who pauses
edits their own Controls and world/weapon FOV. Other graphics settings are shared.
Assign Use health pack for Player 2 in Controls before relying on partner revival.
Co-op campaign coverage and its reported aiming offset remain under investigation.
Live Proton co-op acceptance is still pending.

OPTIONAL HD GAME TEXTURES
Place the separate HD texture ZIP in the extracted package folder and name it
HD-Game-Textures.zip. Do not extract it. Temporarily use these Launch Options:

  -B "tools/import_game.py" textures --pack "HD-Game-Textures.zip" --cancel ".cancel-textures" --interactive

Press Play, acknowledge the start message and wait for the result, then restore
the normal play command from step 6. In the game, enable
Graphics > HD game textures > Apply.
If the option remains gray, installation did not complete; use the failure
instructions below with the texture command. The same HD ZIP works on Windows
and Linux. The unchanged 0.06 pack remains compatible with 0.08; existing users
do not need to reinstall it. HD HUD is included in the main package and still
has known prompt issues.

LANGUAGE
Choose Options > Language in the game, then restart. To select Spanish before
playing, temporarily use these Launch Options and press Play once:

  -B "tools/import_game.py" language --language 4 --interactive

Restore the normal play command afterward. Values are 0 English, 1 French,
2 Italian, 3 German and 4 Spanish. Changing language keeps your other settings.
ZARF's Spanish text and supplemental translations are included in this package.
The first Play after import or upgrade prepares them from your imported game;
the Spanish language command also prepares them. No separate Spanish download
is needed. Spanish uses English voices until you import the Spanish audio from
your European multilingual PS2 ISO (En/Fr/De/Es/It, SLES-53991). Import your Xbox
game first; the PS2 ISO supplies voices, not the base game. If your PS2 disc image
is in a ZIP, extract the ISO first. Place the ISO in the package folder, name it
ps2-europe.iso, and temporarily use these Launch Options:

  -B "tools/import_game.py" voices --iso "ps2-europe.iso" --cancel ".cancel-voices" --interactive

Press Play and acknowledge the start message. The bundled Windows Python and
FFmpeg tools convert the audio locally through Proton; this can take several
minutes. Wait for the completion dialog, select Spanish with the language
command above, then restore the normal play command. The PS2 ISO is no longer
required after a successful voice import. Dialogue and cutscene voices change
only when Spanish is selected; your original imported game files, other
languages and saves stay intact.

IF SETUP OR PLAY FAILS
For a black screen with audio or an FPS counter, record which of those appears.
To collect a Proton log, temporarily replace the normal Launch Options with:

  PROTON_LOG=1 %command% -B "tools/import_game.py" launch --interactive

Reproduce the problem, then close the game. Open your Linux home folder and find
the newest steam-*.log file (the * is the shortcut's numeric ID). Proton writes
logs there by default; if you set PROTON_LOG_DIR yourself, check that folder.
For an import or HD-pack failure, instead put PROTON_LOG=1 %command% before that
command's existing -B arguments, run it once, and collect the same log.

Send the log, exact error text, release version, Linux distribution, GPU/driver,
Proton version and the step that failed. Setup errors are shown in a dialog and
saved as last-setup-error.txt beside tools and runtime when that folder is
writable. Include that file and GameData/last-error.txt if they exist. If Python
cannot start at all, collect the Proton log instead. Restore the normal Launch
Options after collecting the log. Logging is optional troubleshooting, not a
required setting or a rendering fix.

UPDATES AND SAVES
Extract the new release and COPY your GameData folder beside tools and runtime.
Keep a backup. Update the existing Steam shortcut's Target and Start In paths
if the package moved. The next Play updates the runtime, restored UI, shader
seed and bundled Spanish text without replacing saves, settings or HD textures.

Saves: GameData/xbe-runtime/hdd
Settings: GameData/xbe-runtime/settings.toml
Game content: GameData/installed/game_files

Never delete GameData to troubleshoot a Proton prefix. Steam manages its own
prefix for this shortcut. No cache or save folders are removed automatically.
Keep filenames and capitalization as extracted.

ADVANCED ONLY: DIRECT COMMAND-LINE WRAPPER
burner.sh invokes Proton directly. It does NOT set up the Steam Linux Runtime
container, so it may fail on host libraries even when the Steam route works.
Use the Steam instructions above for the recommended setup and for reports.
For users who already have a compatible host environment, run from the package:

  bash burner.sh import "/absolute/path/to/your game.iso"
  bash burner.sh play
  bash burner.sh textures "/absolute/path/to/HD-Game-Textures.zip"
  bash burner.sh voices "/absolute/path/to/Urban Chaos PS2 Europe.iso"
  bash burner.sh language spanish
  bash burner.sh language
  bash burner.sh status
  bash burner.sh --help

Language names: english, french, italian, german, spanish. Status does not launch
the game; exit 0 means imported data is ready, 2 means missing or incompatible.
The wrapper uses .proton beside burner.sh, separate from Steam's prefix. Both
routes use the same portable GameData. The wrapper finds Proton Experimental
in the main Steam library; for another installed version or Steam location:

  BURNER_STEAM="/path/to/Steam" BURNER_PROTON="/path/to/Proton/proton" bash burner.sh play

VERIFICATION LIMITS
Automated checks cover wrapper dispatch, quoting and failure handling. They do
not establish Linux graphics, frame pacing, audio, FMVs or controller support.
Prior-version user feedback confirms one working Linux setup/gameplay session;
0.08's new features, full-campaign behavior and co-op require separate testing.
The shader seed is portable shader data, not a Linux GPU driver cache. No Steam
Deck verification or confirmed fix for every black-screen report is claimed.

Release details and credits: README.txt and licenses/
Proton and logging: https://github.com/ValveSoftware/Proton#runtime-config-options
Steam Linux Runtime: https://github.com/ValveSoftware/steam-runtime
Direct3D 12 translation: https://github.com/HansKristian-Work/vkd3d-proton
