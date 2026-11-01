#!/usr/bin/env bash
# Startet Cult of the Lamb headless in eden-cli mit virtuellem zweitem Bildschirm.
#
#   headless.sh <save-dir|-> <btn-file|-> <out-prefix> [cmd-file] [seconds]
#
# save-dir   Ordner mit slot_0MP usw. (wird vor dem Start in den Profil-Save kopiert), "-" = unverändert
# btn-file   Eingabeskript (PORTING_A_GAME.md §6), wird nach dem Boot atomar als <prefix>.btn abgelegt
# out-prefix Präfix für Log, Screenshots (<prefix>-screen1.png / -screen2.png) und Konsolen-Ausgabe
# cmd-file   optionale Befehle für die DSMod-Konsole (EDEN_DSMOD_CMD)
# seconds    Laufzeit, danach Screenshot und Ende (Standard 120)
#
# Eden-Daten liegen getrennt vom normalen Profil unter ~/cotl-run/data (XDG_DATA_HOME).
set -euo pipefail

SAVE="$1"; BTN="$2"; PREFIX="$3"; CMD="${4:-}"; SECS="${5:-120}"
DEPS="${COTL_DEPS:-$HOME/cotl-deps}"
CLI="$(find "$DEPS/eden-duo/build/bin" -maxdepth 1 -name 'eden-cli*' -type f | head -1)"
PROJ="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$PROJ/game/Cult of the Lamb [01002E7016C46000][v0][Base].nsp"
export XDG_DATA_HOME="$HOME/cotl-run/data"
EDEN="$XDG_DATA_HOME/eden"
TITLE=01002E7016C46000

mkdir -p "$(dirname "$PREFIX")"

if [ "$SAVE" != "-" ]; then
  # Save-Pfad wie SaveDataFactory::GetFullPath: {user_id[1]:016X}{user_id[0]:016X},
  # user_id = erste UUID (u128, little endian) in profiles.dat @ 0x10
  prof="$EDEN/nand/system/save/8000000000000010/su/avators/profiles.dat"
  if [ ! -f "$prof" ]; then
    echo "Kein Eden-Profil – Spiel einmal ohne Save starten (save-dir '-')." >&2
    exit 1
  fi
  uid="$(python3 -c "import struct,sys; a,b=struct.unpack_from('<QQ',open(sys.argv[1],'rb').read(),0x10); print(f'{b:016X}{a:016X}')" "$prof")"
  dest="$EDEN/nand/user/save/0000000000000000/$uid/$TITLE"
  rm -rf "$dest"
  mkdir -p "$dest"
  cp -r "$SAVE"/. "$dest"/
fi

rm -f "$PREFIX".btn "$PREFIX".shot "$PREFIX"-screen1.png "$PREFIX"-screen2.png
env_args=(EDEN_VSYNC=0)
[ -n "$CMD" ] && env_args+=(EDEN_DSMOD_CMD="$CMD")

# Eigene Konfiguration (Renderer usw.), siehe docs/re-notes.md
CONF="$HOME/cotl-run/sdl-config.ini"
conf_args=()
[ -f "$CONF" ] && conf_args=(-c "$CONF")

# --null-render: WSL hat keinen GPU-Vulkan-Treiber, lavapipe stürzt ab (docs/re-notes.md).
# Das Spielbild (screen1) bleibt dadurch schwarz; Spiellogik und screen2 laufen normal.
env "${env_args[@]}" "$CLI" "${conf_args[@]}" --null-render --aux-virtual --screenshot-prefix "$PREFIX" \
  --filter "*:Info" "$GAME" > "$PREFIX.log" 2>&1 &
pid=$!

if [ "$BTN" != "-" ]; then
  # Eingaben erst nach dem Boot schicken (Titelbild nach ~35 s mit Null-Renderer), sonst gehen sie verloren
  sleep "${BOOT_WAIT:-40}"
  cp "$BTN" "$PREFIX.btn.tmp" && mv "$PREFIX.btn.tmp" "$PREFIX.btn"
fi

sleep "$SECS"
touch "$PREFIX.shot"
for _ in $(seq 1 30); do [ -f "$PREFIX-screen2.png" ] && break; sleep 1; done
kill "$pid" 2>/dev/null || true
wait "$pid" 2>/dev/null || true
ls -l "$PREFIX"-screen*.png 2>/dev/null || echo "Keine Screenshots – siehe $PREFIX.log" >&2
