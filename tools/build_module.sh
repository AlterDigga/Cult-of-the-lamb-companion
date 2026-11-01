#!/usr/bin/env bash
# Baut das Modul für linux-x86_64 und android-arm64-v8a und strippt beide (in WSL ausführen).
# Ergebnis: build-linux/01002E7016C46000.so, build-android/01002E7016C46000.so
set -euo pipefail

PROJ="$(cd "$(dirname "$0")/.." && pwd)"
DEPS="${COTL_DEPS:-$HOME/cotl-deps}"
NDK="$DEPS/android-ndk-r28c"
TITLE=01002E7016C46000
STRIP="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"

cmake -S "$PROJ" -B "$PROJ/build-linux" -G Ninja -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build "$PROJ/build-linux" --target dsmod-cotl cotl-tests
"$PROJ/build-linux/cotl-tests" > /dev/null

cmake -S "$PROJ" -B "$PROJ/build-android" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_static \
  -DBUILD_TESTING=OFF > /dev/null
cmake --build "$PROJ/build-android" --target dsmod-cotl

"$STRIP" --strip-all "$PROJ/build-linux/$TITLE.so"
"$STRIP" --strip-all "$PROJ/build-android/$TITLE.so"
