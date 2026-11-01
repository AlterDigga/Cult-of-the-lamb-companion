#!/usr/bin/env bash
# Wrapper: dis.sh <Methodenname|0xRVA> [anzahl] – setzt PYTHONPATH für capstone/lz4 aus ~/cotl-deps/py
P="$HOME/cotl-deps/py/root"
export PYTHONPATH="$P/usr/lib/python3/dist-packages" LD_LIBRARY_PATH="$P/usr/lib/x86_64-linux-gnu"
W="$(cd "$(dirname "$0")/../.." && pwd)/work"
exec python3 "$(dirname "$0")/disasm.py" "$W/exefs/main" "$W/il2cpp/script.json" "$@"
