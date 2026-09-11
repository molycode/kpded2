# kpded2

kpded2 is a version of the R1Q2 Quake 2 server that has been modified and enhanced for [Kingpin: Life of Crime](https://www.wikipedia.org/wiki/Kingpin:_Life_of_Crime).

`kpded2.txt` documents the server's options and commands. This fork adds a CMake build and fixes
what a modern compiler rejects; `todo.md` records what is still parked.

## Lineage

- Quake 2, id Software, released under the GPL v2 in 2001.
- [R1Q2](https://github.com/r1ch/r1q2) by Richard "r1ch" Stanway, whose history this repository
  carries up to build `b8012` (2011).
- kpded2 by MonkeyHarris, which strips R1Q2 to a dedicated server and adapts it to Kingpin.

See `LICENSE`. Every source file carries id's GPL v2 header.

## Building on Linux

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

Presets are `linux-gcc-{debug,release}` and `linux-clang-{debug,release}`, using whichever GCC or
Clang is on `PATH`. CMake 4.3.2 or newer is required.

The server is 32-bit because the game library it loads is, so a 64-bit host needs the 32-bit
toolchain and libraries:

```bash
sudo apt install gcc-multilib zlib1g-dev:i386
```

A missing 32-bit zlib shows up as `Could NOT find ZLIB (missing: ZLIB_LIBRARY)` at configure time.

GCC is what ships. A Clang build is a diagnostic second opinion and is much noisier - see `todo.md`.
A GCC Release build also writes a stripped copy to `build/<preset>/ship/kpded2`, carrying the same
GNU build id as the unstripped binary beside it, so a crash in the shipped server still symbolises.

> Changing anything in `cmake/toolchains/` requires deleting `build/` first. `CMAKE_C_FLAGS_INIT`
> only seeds the cache on a build tree's **first** configure, so an existing tree silently keeps the
> old flags.

`Makefile` is upstream's build and still works; it builds `q2ded2` as well, which the CMake build
deliberately does not. See `todo.md`.

## Building on Windows

`kpded2.sln` is the only proven Windows entry point. The `windows-msvc-*` presets exist but have
never been run - see `todo.md` before relying on them.
