Urban Chaos: Riot Response - Burner Edition
EXPERIMENTAL LINUX / PROTON PACKAGE

This package runs the same Windows x64 game through Proton. It is not a native
Linux executable. Linux gameplay, rendering, audio, FMVs and controllers have
not yet been verified on Linux hardware. Please treat this as a testing build.

REQUIREMENTS
- Linux x86-64, Bash, Steam, and Proton Experimental installed through Steam.
- A Vulkan-capable GPU and working Vulkan drivers supported by your Proton
  version. Proton translates Direct3D 12 through vkd3d-proton.
- Your supported original Xbox ISO, or an existing portable GameData folder.
- A writable extracted folder on a Linux filesystem. Do not run inside an
  archive. The complete runtime, tools and licenses folders must stay together.

No Proton, Wine, drivers or game-disc content are downloaded or installed by
our launcher. Proton manages its own compatibility prefix in .proton beside
burner.sh. The Windows .NET setup window is not required for these commands.

FIRST SETUP
Extract the Linux archive. Open a terminal in that extracted folder, then run:

  ./burner.sh import "/absolute/path/to/your game.iso"
  ./burner.sh play

Both the verified full-disc ISO and extracted .xiso.iso layouts are accepted.
The same import validation as the Windows release is used. Import and texture
installation print progress messages. Ctrl+C interrupts the running command.
If your extraction tool lost the executable permission, use:

  bash burner.sh import "/absolute/path/to/your game.iso"
  bash burner.sh play

OPTIONS
Install the separate HD game texture download without extracting that ZIP:

  ./burner.sh textures "/absolute/path/to/HD-Game-Textures.zip"

Then enable Graphics > HD game textures in the game. HD HUD is bundled; it is
separate from the optional game texture pack and still has known prompt issues.

Select a language before playing, or display the current selection:

  ./burner.sh language english
  ./burner.sh language spanish
  ./burner.sh language

Supported names: english, french, italian, german, spanish. Spanish text data
from ZARF and supplemental translations are bundled in this package. The first
Play after import or upgrade prepares the translated files automatically from
your imported game; selecting Spanish with the language command also prepares
them. No separate Spanish download is needed. Voices remain in English.
Changing the language preserves other settings. Restart after changes.

Check whether the original game data has been imported:

  ./burner.sh status

A successful status exits with 0; missing/incompatible game data exits with 2.
It does not start the game. For all commands and configuration help:

  ./burner.sh --help

PROTON LOCATION
The launcher looks for Steam in the usual desktop and Flatpak locations, then
uses steamapps/common/Proton - Experimental/proton. If Proton is in another
Steam library or you want to test another installed version:

  BURNER_PROTON="/path/to/Proton/proton" ./burner.sh play

For a nonstandard Steam installation, also provide its client folder:

  BURNER_STEAM="/path/to/Steam" BURNER_PROTON="/path/to/Proton/proton" ./burner.sh play

These direct commands depend on the host libraries required by your Proton
installation. If Proton reports missing Linux libraries, use the Steam launch
method below so Steam supplies its compatibility runtime. No Windows DLL
overrides, graphics workarounds or lower-fidelity settings are forced.

STEAM LAUNCH METHOD
Add tools/python/python.exe as a Non-Steam Game. In its Properties:
1. Set Start In to this extracted package folder.
2. Set Launch Options to: -B "tools/import_game.py" launch
3. In Compatibility, force Proton Experimental.
4. Start the shortcut. Keep using this same shortcut for the same Proton prefix.

For first import through that shortcut, temporarily change Launch Options to:
  -B "tools/import_game.py" import --iso "Z:\home\YOUR_USER\game.iso" --cancel ".cancel-import"
Replace the example ISO path with the location of your own ISO. Restore the
launch options from step 2 after import. For the separate HD pack use:
  -B "tools/import_game.py" textures --pack "Z:\home\YOUR_USER\HD-Game-Textures.zip" --cancel ".cancel-textures"
Steam sandbox/Flatpak users must allow access to the package and input files.
This Steam method uses Steam's prefix; the shell method uses the local .proton
prefix. Both use the same GameData folder for game saves and settings.

UPDATES AND PORTABILITY
Copy your GameData folder beside burner.sh in the new release; keep a backup.
The verified runtime, restored UI and portable shader seed are updated on Play.
The refreshed 0.07 package also includes the previously omitted Spanish text
data. Replace the release files while keeping GameData, or copy GameData into
a fresh extraction. The next Play or explicit Spanish language selection
prepares it locally without replacing saves, other settings or HD textures.
The same optional HD texture ZIP works with Windows and this Proton package.
Saves: GameData/xbe-runtime/hdd
Settings: GameData/xbe-runtime/settings.toml
Game content: GameData/installed/game_files
Never delete GameData to troubleshoot a Proton prefix. No cache or save folders
are removed automatically. Keep filenames and capitalization as extracted.

TESTING LIMITS
Command quoting, paths with spaces, installer dispatch, private-prefix behavior
and failure propagation have automated checks. Those checks do not establish
working Linux graphics, frame pacing, audio, video codecs or controller support.
The bundled shader seed is portable game shader data, not a Linux driver cache.
Please report distro, GPU, driver, Proton version and the failing step when
reporting Linux problems. No promise of Steam Deck verification is made.

Details and credits for the game release are in README.txt and licenses/.
Proton: https://github.com/ValveSoftware/Proton
Direct3D 12 translation: https://github.com/HansKristian-Work/vkd3d-proton
