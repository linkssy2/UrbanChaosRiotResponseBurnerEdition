#!/usr/bin/env bash
# Experimental Linux entry point; the verified Windows worker owns installation and saves.
set -euo pipefail

fail() { printf 'Burner Edition: %s\n' "$*" >&2; exit 1; }
usage() {
    cat <<'HELP'
Urban Chaos: Riot Response - experimental Linux / Proton package

Advanced direct-Proton wrapper. Recommended setup: use Steam as described in
tools/README-linux.md. This wrapper does not set up the Steam Linux Runtime container.

  ./burner.sh use-iso "/path/to/your Xbox game.iso"
  ./burner.sh import "/path/to/your Xbox game.iso"
  ./burner.sh use-installation "/path/to/previous package or GameData"
  ./burner.sh play
  ./burner.sh verify
  ./burner.sh auto-setup
  ./burner.sh textures "/path/to/HD-Game-Textures.zip"
  ./burner.sh voices "/path/to/Urban Chaos PS2 Europe.iso"
  ./burner.sh status
  ./burner.sh diagnostics [on|off]
  ./burner.sh startup [on|off]
  ./burner.sh check-updates
  ./burner.sh update
  ./burner.sh recover-update
  ./burner.sh language [english|french|italian|german|spanish]

Place one supported Xbox ISO/XISO in assets/ for automatic first-time setup.
Use-iso keeps that image required. Import copies it once; use-installation reuses
existing game data, saves and add-ons without copying them.

Use an installed Proton version. To choose it explicitly:
  BURNER_PROTON="/path/to/Proton/proton" ./burner.sh play
Experimental is the default when installed, but is not required. Otherwise a
single installed Proton in the main Steam library is selected automatically.
For a nonstandard Steam installation, also set BURNER_STEAM to its folder.
Spanish voices require your European multilingual PS2 ISO (SLES-53991).
Import your Xbox game first, then select Spanish after importing the voices.
Startup on skips intro videos and Press Start to profile selection next launch.
It defaults to off; mission videos are unchanged.
This runs the Windows game through Proton; it is not a native Linux port.
HELP
}

action=${1:-play}
if (( $# )); then shift; fi
case "$action" in
    help|-h|--help) usage; exit 0 ;;
    import|use-iso|textures|voices) [[ $# == 1 ]] || fail "$action requires exactly one file path." ;;
    use-installation) [[ $# == 1 ]] || fail 'use-installation requires one package or GameData folder.' ;;
    play|status|verify|auto-setup|check-updates|update|recover-update) [[ $# == 0 ]] || fail "$action does not take arguments." ;;
    language) [[ $# -le 1 ]] || fail 'language accepts one language name.' ;;
    diagnostics) [[ $# -le 1 ]] || fail 'diagnostics accepts on or off.' ;;
    startup) [[ $# -le 1 ]] || fail 'startup accepts on or off.' ;;
    *) usage >&2; fail "Unknown command: $action" ;;
esac

language=''
if [[ $action == language && $# == 1 ]]; then
    case "${1,,}" in
        english|0) language=0 ;;
        french|1) language=1 ;;
        italian|2) language=2 ;;
        german|3) language=3 ;;
        spanish|4) language=4 ;;
        *) fail 'Choose english, french, italian, german or spanish.' ;;
    esac
fi

logging=''
if [[ $action == diagnostics && $# == 1 ]]; then
    case "$1" in on|off) logging=$1 ;; *) fail 'Choose diagnostics on or off.' ;; esac
fi
skip_startup=''
if [[ $action == startup && $# == 1 ]]; then
    case "$1" in on|off) skip_startup=$1 ;; *) fail 'Choose skip startup on or off.' ;; esac
fi

# Resolve the script itself so a desktop symlink keeps the real package/prefix.
entry=$(readlink -f -- "${BASH_SOURCE[0]}") || fail 'Could not locate the launcher. Run burner.sh from the extracted package.'
root=$(CDPATH= cd -- "$(dirname -- "$entry")" && pwd -P)
# Run updates outside Proton; replacing the shell through Wine loses Unix modes.
updater="$root/tools/release_update.py"
if [[ -f "$root/.launcher-update/transaction.json" ]]; then
    [[ ! -L "$root/.launcher-update" && ! -L "$root/.launcher-update/runner" && ! -L "$root/.launcher-update/runner/release_update.py" ]] || fail 'Linked recovery files are not supported.'
    updater="$root/.launcher-update/runner/release_update.py"
    command -v python3 >/dev/null || fail 'Install Python 3.10 or newer to recover this update.'
    python3 -B "$updater" --recover "$root" || fail 'Update recovery failed. Keep the backup and read the error above.'
fi
if [[ $action == update || $action == check-updates || $action == recover-update ]]; then
    command -v python3 >/dev/null || fail 'Install Python 3.10 or newer to update the launcher.'
    [[ $action != recover-update ]] || exit 0
    option=--download
    [[ $action != check-updates ]] || option=--check
    exec python3 -B "$root/tools/release_update.py" "$option" "$root"
fi
for payload in tools/python/python.exe tools/import_game.py tools/runtime-manifest.json runtime/UrbanChaosXbeTrial.exe; do
    [[ -f "$root/$payload" ]] || fail "Missing $payload. Extract the entire Linux package again."
done
input=''
if [[ $action == import || $action == use-iso || $action == textures || $action == voices ]]; then
    [[ -f $1 ]] || fail "File not found: $1"
    input=$(CDPATH= cd -- "$(dirname -- "$1")" && pwd -P)/$(basename -- "$1")
elif [[ $action == use-installation ]]; then
    [[ -d $1 ]] || fail "Folder not found: $1"
    input=$(CDPATH= cd -- "$1" && pwd -P)
fi

steam_root=${BURNER_STEAM:-${STEAM_COMPAT_CLIENT_INSTALL_PATH:-}}
if [[ -z $steam_root ]]; then
    for candidate in "$HOME/.steam/root" "$HOME/.local/share/Steam" "$HOME/.var/app/com.valvesoftware.Steam/data/Steam"; do
        if [[ -d "$candidate/steamapps" ]]; then steam_root=$candidate; break; fi
    done
fi
[[ -n $steam_root && -d "$steam_root/steamapps" ]] || fail 'Steam was not found. Set BURNER_STEAM to your Steam installation folder.'
steam_root=$(CDPATH= cd -- "$steam_root" && pwd -P)
proton=${BURNER_PROTON:-}
if [[ -z $proton ]]; then
    experimental="$steam_root/steamapps/common/Proton - Experimental/proton"
    if [[ -f $experimental && -x $experimental ]]; then
        proton=$experimental
    else
        choices=()
        for candidate in "$steam_root"/steamapps/common/Proton*/proton; do
            [[ ! -f $candidate || ! -x $candidate ]] || choices+=("$candidate")
        done
        [[ ${#choices[@]} != 1 ]] || proton=${choices[0]}
    fi
fi
[[ -n $proton && -f $proton && -x $proton ]] || fail 'Choose an installed Proton with BURNER_PROTON="/path/to/Proton/proton", or use Steam and choose its compatibility tool. Experimental is optional.'
proton=$(CDPATH= cd -- "$(dirname -- "$proton")" && pwd -P)/$(basename -- "$proton")

# Keep compatibility state separate from saves, and never follow a redirected prefix.
[[ ! -L "$root/.proton" ]] || fail 'The .proton folder must not be a symbolic link.'
umask 077
mkdir -p -- "$root/.proton"
export STEAM_COMPAT_DATA_PATH="$root/.proton"
export STEAM_COMPAT_CLIENT_INSTALL_PATH="$steam_root"
export SteamAppId=${SteamAppId:-0}
export SteamGameId=${SteamGameId:-0}
cd -- "$root"

compat_path() {
    local converted
    converted=$("$proton" getcompatpath "$1") || return
    converted=${converted%$'\r'}
    [[ -n $converted ]] || fail 'Proton could not convert a file path.'
    printf '%s' "$converted"
}
win_root=$(compat_path "$root")
# Omit --data so every entry point uses the shared installation selection.
args=("$action")
if [[ -n $input ]]; then
    win_input=$(compat_path "$input")
    option=--iso
    [[ $action != textures ]] || option=--pack
    [[ $action != use-installation ]] || option=--installation
    args+=("$option" "$win_input")
    if [[ $action != use-installation ]]; then
        # This invocation owns a fresh cancellation-marker name.
        cancel=".cancel-$$-$RANDOM"
        while [[ -e "$root/.proton/$cancel" ]]; do cancel=".cancel-$$-$RANDOM"; done
        args+=(--cancel "$win_root\\.proton\\$cancel")
    fi
fi
if [[ -n $language ]]; then args+=(--language "$language"); fi
if [[ -n $logging ]]; then args+=(--logging "$logging"); fi
if [[ -n $skip_startup ]]; then args+=(--skip-startup "$skip_startup"); fi
[[ $action != play ]] || args[0]=launch
run_worker() {
    "$proton" waitforexitandrun "$win_root\\tools\\python\\python.exe" -B "$win_root\\tools\\import_game.py" "${args[@]}"
}
run_logged() {
    local folder="$root/GameData/logs" marker="$root/GameData/.diagnostic-logging" file
    if [[ -d "$root/GameData" && ! -L "$root/GameData" && -f "$marker" && ! -L "$marker" &&
          $(stat -c %h -- "$marker") == 1 && $(stat -c %s -- "$marker") == 2 && $(cat -- "$marker") == 1 &&
          ! -L "$folder" ]] && mkdir -p -- "$folder"; then
        file="$folder/proton-$(date -u +%Y%m%dT%H%M%S)-$$-$RANDOM.log"
        # Exclusive creation and bounded draining preserve output and the worker's exit code.
        if (set -o noclobber; : > "$file") 2>/dev/null; then
            local prior=("$folder"/proton-????????T??????-*.log) i
            for ((i=0; i<${#prior[@]}-30; i++)); do
                [[ ! -f ${prior[i]} || -L ${prior[i]} || $(stat -c %h -- "${prior[i]}") != 1 ]] || rm -- "${prior[i]}"
            done
            run_worker 2>&1 | tee >(cat >&2) | { head -c 8388608; cat > /dev/null; } > "$file"
            return
        fi
    fi
    run_worker
}
if run_logged; then
    exit 0
else
    result=$?
    if [[ $action != status || $result != 2 ]]; then
        printf 'Burner Edition: %s failed (exit %s). Read the error above and tools/README-linux.md. If Proton cannot start directly, use the recommended Steam shortcut so Steam supplies its Linux Runtime.\n' "$action" "$result" >&2
    fi
    exit "$result"
fi
