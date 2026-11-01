#!/usr/bin/env bash
# Erzeugt native/atlas_bundles_1_5_27.inc: SpriteAtlas-Name → Addressables-Bundle (in WSL ausführen).
# Dauer ~2 min (liest die Verzeichnisse aller ~10.000 Bundles).
set -euo pipefail
PROJ="$(cd "$(dirname "$0")/.." && pwd)"
DEPS="${COTL_DEPS:-$HOME/cotl-deps}"
g++ -std=c++20 -O2 -I "$DEPS/eden-duo-companions/native/modules" -I "$DEPS/eden-duo/src" \
  "$PROJ/tools/re/bundlescan.cpp" -L"$PROJ/build-linux" -lce_unity -o /tmp/cotl-bundlescan
{
  echo "// Generiert von tools/gen_atlas_bundles.sh (Cult of the Lamb 1.5.27). Nicht bearbeiten."
  /tmp/cotl-bundlescan "$PROJ/work/romfs/Data/StreamingAssets/aa/Switch" |
    sed -n 's/^\(.*\.bundle\) atlas=\(.*\)$/COTL_ATLAS("\2", "\1")/p' | sort -u
} > "$PROJ/native/atlas_bundles_1_5_27.inc"
grep -c COTL_ATLAS "$PROJ/native/atlas_bundles_1_5_27.inc"
