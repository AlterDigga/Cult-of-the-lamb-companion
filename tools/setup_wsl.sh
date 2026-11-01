#!/usr/bin/env bash
# Richtet die Build-Abhängigkeiten in WSL (Ubuntu 26.04) unter ~/cotl-deps ein:
# Eden Duo v1.3.2.2 (SDK-Header + eden-cli mit Dev-Tools), eden-duo-companions, Android NDK r28c.
# Wiederholbar: vorhandene Teile werden übersprungen.
set -euo pipefail

DEPS="${COTL_DEPS:-$HOME/cotl-deps}"
EDEN_TAG="v1.3.2.2"
NDK_ZIP="android-ndk-r28c-linux.zip"
JOBS="${JOBS:-4}"   # WSL hat ~7 GB RAM; mehr Jobs lassen den Linker ins Swapping laufen

mkdir -p "$DEPS"
cd "$DEPS"

if [ ! -d eden-duo ]; then
  git clone --depth 1 --branch "$EDEN_TAG" --recurse-submodules --shallow-submodules \
    https://github.com/igawa6/eden-duo.git
fi
if [ ! -d eden-duo-companions ]; then
  git clone --depth 1 https://github.com/igawa6/eden-duo-companions.git
fi

# LZ4-Quellcode für ce_unity (Unity-Bundles); Eden nutzt unter Linux die System-Bibliothek
if [ ! -d lz4 ]; then
  git clone --depth 1 --branch v1.10.0 https://github.com/lz4/lz4.git
fi

if [ ! -d android-ndk-r28c ]; then
  curl -fL -o "$NDK_ZIP" "https://dl.google.com/android/repository/$NDK_ZIP"
  unzip -q "$NDK_ZIP"
  rm "$NDK_ZIP"
fi

cmake -S eden-duo -B eden-duo/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_QT=OFF -DYUZU_CMD=ON -DYUZU_TESTS=OFF \
  -DENABLE_WERROR=OFF \
  -DEDEN_DSMOD_BUILD_DEV_TOOLS=ON
# Das Ziel heißt yuzu-cmd, die erzeugte Datei eden-cli
cmake --build eden-duo/build --target yuzu-cmd --parallel "$JOBS"

echo "eden-cli: $(find "$DEPS/eden-duo/build/bin" -maxdepth 1 -name 'eden-cli*' -type f | head -1)"
echo "NDK:      $DEPS/android-ndk-r28c"
