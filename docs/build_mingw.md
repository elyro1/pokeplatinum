# Build Windows binaries on Linux

From a checkout that can already build with `make linux`, run:

```sh
make mingw
```

Additional prerequisites are Python 3.12+, CMake, curl, Ninja, and the
`x86_64-w64-mingw32` GCC/G++ and binutils toolchain. On Arch-based systems the
cross-toolchain package is `mingw-w64-gcc`. Wine is optional for running the result.

The first run downloads pinned SDL2 MinGW development files and builds ENet and
SoundTouch for Windows under `build_win64/local-deps`. It does not install system
packages. Subsequent runs reuse those downloads and compile only changed inputs.

The runnable Windows files are written to:

```text
build_win64/pokeplatinum/release_win64/
    main.exe
    SDL2.dll
    firmware.bin
    header.bin
```

ENet, SoundTouch and the GCC/C++ threading runtimes are linked statically. Keep
SDL2.dll, firmware.bin and header.bin beside main.exe. Supply your dumped ROMs as
described in the README.

To choose parallelism or build directories:

```sh
python3 tools/scripts/build_mingw.py -j 8 \
    --native-build build_linux --build build_win64
```

The script first builds the native Linux tools and assets. It derives a separate
Ninja graph from Meson's source lists and archive/link ordering, switches to the
Windows SDK defines, omits the Linux-only window icon source, and compiles all game
and SDK objects with MinGW. The native preprocessed trainer AI assembly is also
assembled into a Windows object. Generated headers and resource patches are shared
with the native build; Linux binaries and libraries are never linked into main.exe.

This is a Linux cross-build path. `make win64` and Windows CI continue to use MSYS2.
