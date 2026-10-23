#!/usr/bin/env bash
# Experimental Linux entry point; the verified Windows worker owns installation and saves.
set -euo pipefail

fail() { printf 'Burner Edition: %s\n' "$*" >&2; exit 1; }
usage() {
    cat <<'HELP'
Urban Chaos: Riot Response - experimental Linux / Proton package

  ./burner.sh import "/path/to/your Xbox game.iso"
  ./burner.sh play
  ./burner.sh textures "/path/to/HD-Game-Textures.zip"
  ./burner.sh status
  ./burner.sh language [english|french|italian|german|spanish]

Install Proton Experimental through Steam first. For another installed Proton:
  BURNER_PROTON="/path/to/Proton/proton" ./burner.sh play
For a nonstandard Steam installation, also set BURNER_STEAM to its folder.
This runs the Windows game through Proton; it is not a native Linux port.
HELP
}

action=${1:-play}
if (( $# )); then shift; fi
case "$action" in
    help|-h|--help) usage; exit 0 ;;
    import|textures) [[ $# == 1 ]] || fail "$action requires exactly one file path." ;;
    play|status) [[ $# == 0 ]] || fail "$action does not take arguments." ;;
    language) [[ $# -le 1 ]] || fail 'language accepts one language name.' ;;
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

root=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
for payload in tools/python/python.exe tools/import_game.py tools/runtime-manifest.json runtime/UrbanChaosXbeTrial.exe; do
    [[ -f "$root/$payload" ]] || fail "Missing $payload. Extract the entire Linux package again."
done
input=''
if [[ $action == import || $action == textures ]]; then
    [[ -f $1 ]] || fail "File not found: $1"
    input=$(CDPATH= cd -- "$(dirname -- "$1")" && pwd -P)/$(basename -- "$1")
fi

steam_root=${BURNER_STEAM:-${STEAM_COMPAT_CLIENT_INSTALL_PATH:-}}
if [[ -z $steam_root ]]; then
    for candidate in "$HOME/.steam/root" "$HOME/.local/share/Steam" "$HOME/.var/app/com.valvesoftware.Steam/data/Steam"; do
        if [[ -d "$candidate/steamapps" ]]; then steam_root=$candidate; break; fi
    done
fi
[[ -n $steam_root && -d "$steam_root/steamapps" ]] || fail 'Steam was not found. Set BURNER_STEAM to your Steam installation folder.'
steam_root=$(CDPATH= cd -- "$steam_root" && pwd -P)
proton=${BURNER_PROTON:-"$steam_root/steamapps/common/Proton - Experimental/proton"}
[[ -f $proton && -x $proton ]] || fail 'Proton was not found. Install Proton Experimental in Steam, or set BURNER_PROTON to an installed proton executable.'
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
args=("$action" --data "$win_root\\GameData")
if [[ -n $input ]]; then
    win_input=$(compat_path "$input")
    option=--iso
    [[ $action != textures ]] || option=--pack
    # The shared importer expects a cancellation marker; this invocation owns a fresh name.
    cancel=".cancel-$$-$RANDOM"
    while [[ -e "$root/.proton/$cancel" ]]; do cancel=".cancel-$$-$RANDOM"; done
    args+=("$option" "$win_input" --cancel "$win_root\\.proton\\$cancel")
fi
if [[ -n $language ]]; then args+=(--language "$language"); fi
[[ $action != play ]] || args[0]=launch
exec "$proton" waitforexitandrun "$win_root\\tools\\python\\python.exe" -B "$win_root\\tools\\import_game.py" "${args[@]}"
