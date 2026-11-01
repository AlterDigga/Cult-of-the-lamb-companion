#!/usr/bin/env bash
# Baut das Companion-Paket (in WSL ausführen).
#
#   build.sh [--no-module] [--version X.Y.Z]
#
# Ergebnis: out/01002E7016C46000.dsmod.zip. Zusätzlich wird das Paket für eden-cli in das
# getrennte Dev-Profil installiert (~/cotl-run/data/eden/load/01002E7016C46000/cotl).
set -euo pipefail

PROJ="$(cd "$(dirname "$0")/.." && pwd)"
DEPS="${COTL_DEPS:-$HOME/cotl-deps}"
PKG="$PROJ/package/CultOfTheLamb"
TITLE=01002E7016C46000
# 32 Byte Build-ID: 16 Byte aus main + 16 Null-Bytes
BUILD_ID="316A4D4881A9F1E077A07262E917917C$(printf '0%.0s' $(seq 1 32))"
VERSION="0.1.0"
WITH_MODULE=1

while [ $# -gt 0 ]; do
  case "$1" in
    --no-module) WITH_MODULE=0 ;;
    --version) VERSION="$2"; shift ;;
    *) echo "Unbekannte Option $1" >&2; exit 2 ;;
  esac
  shift
done

python3 "$PROJ/tools/gen_manifest.py" > /dev/null

module_args=()
if [ "$WITH_MODULE" = 1 ]; then
  "$PROJ/tools/build_module.sh"
  module_args=(--module "linux-x86_64=$PROJ/build-linux/$TITLE.so"
               --module "android-arm64-v8a=$PROJ/build-android/$TITLE.so"
               --abi 1 --build-id "$BUILD_ID")
fi

mkdir -p "$PROJ/out"
python3 "$DEPS/eden-duo-companions/tools/build_dualscreen_package.py" \
  --package "$PKG" --output "$PROJ/out" --version "$VERSION" "${module_args[@]}"

# Dev-Installation für eden-cli: das Archiv entpacken, damit die Modul-Hashes zu den Dateien passen
DEV="$HOME/cotl-run/data/eden/load/$TITLE/cotl"
rm -rf "$DEV"
mkdir -p "$DEV"
python3 -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" \
  "$PROJ/out/$TITLE.dsmod.zip" "$DEV"
echo "Paket: $PROJ/out/$TITLE.dsmod.zip"
echo "Dev-Installation: $DEV"
