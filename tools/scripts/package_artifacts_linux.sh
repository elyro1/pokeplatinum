#!/bin/bash
set -euo pipefail

BUILDDIR=build_linux/pokeplatinum
APPDIR="$BUILDDIR/pokeplatinum.AppDir"
RELEASEDIR="$BUILDDIR/release_linux"

# Reuse the dependencies collected by linuxdeploy for the AppImage.
# Dereference symlinks because actions/upload-artifact does not preserve them.
mkdir -p "$RELEASEDIR/lib"
cp -Lr "$APPDIR/usr/lib/." "$RELEASEDIR/lib/"
cp "$BUILDDIR/firmware.bin" "$RELEASEDIR/firmware.bin"
cp "$BUILDDIR/header.bin" "$RELEASEDIR/header.bin"
cp "$BUILDDIR/main" "$RELEASEDIR/main"
cp "$BUILDDIR/pokeplatinum.AppImage" "$RELEASEDIR/pokeplatinum.AppImage"
patchelf --set-rpath '$ORIGIN/lib' "$RELEASEDIR/main"

# Check the packaged executable, including its transitive dependencies.
dependencies=$(LD_LIBRARY_PATH= ldd "$RELEASEDIR/main")
printf '%s\n' "$dependencies"
if [[ "$dependencies" == *"not found"* ]]; then
  echo "The Linux release has missing shared libraries." >&2
  exit 1
fi
