# kpded2

kpded2 is a version of the R1Q2 Quake 2 server that has been modified and enhanced for [Kingpin: Life of Crime](https://www.wikipedia.org/wiki/Kingpin:_Life_of_Crime).

`kpded2.txt` documents the server's options and commands. This fork adds a CMake build and fixes
what a modern compiler rejects; `todo.md` records what is still parked.

## Lineage

- Quake 2, id Software, released under the GPL v2 in 2001.
- [R1Q2](https://github.com/r1ch/r1q2) by r1ch, whose history this repository
  carries up to build `b8012` (2011).
- kpded2 by MonkeyHarris, which strips R1Q2 to a dedicated server and adapts it to Kingpin.

See `LICENSE`. Every source file carries id's GPL v2 header.

## Running a server

**The server is a 32-bit binary and a 64-bit host will not run it out of the box.** Kingpin's game
library is i386, the server loads it into its own process, and nothing about that can be changed, so
the server has to be i386 too. On a 64-bit Debian or Ubuntu host:

```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install libc6:i386
```

Without it the server does not start, and **the error blames the binary rather than the missing
library** - what is absent is the 32-bit loader `/lib/ld-linux.so.2`, which the kernel needs before
any of the executable runs:

```
$ ./kpded2
bash: ./kpded2: cannot execute: required file not found
```

Older shells word it `No such file or directory`, which is worse, since the file is plainly there.
`file kpded2` showing `ELF 32-bit` next to a host missing `/lib/ld-linux.so.2` is the diagnosis.
**`ldd` will not help** - with no matching loader it either fails or silently answers using the
host's own, so it can report a clean set of dependencies for a binary that cannot start at all.

`libc6:i386` is the floor and cannot be linked away: the server `dlopen`s the game library, and a
statically linked glibc cannot do that reliably. Everything above that floor can be - a build linking
a static zlib needs nothing but `libc6:i386` (see below).

The server needs only `kpded2` and the game's `main/` directory beside it. No renderer, no sound, no
X11.

`+set public 0` keeps a server off the master list. **Do this when testing**: the Kingpin build
announces to `master.kingpin.info` by default, so an unconfigured throwaway server advertises itself
publicly under whatever address its heartbeat came from. A server stopped with SIGTERM de-registers
on the way out; one killed outright lingers until the master expires it.

## Building on Linux

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

Presets are `linux-gcc-{debug,release}` and `linux-clang-{debug,release}`, using whichever GCC or
Clang is on `PATH`. CMake 4.3.2 or newer is required. Building 32-bit on a 64-bit host needs the
32-bit toolchain and libraries:

```bash
sudo apt install gcc-multilib zlib1g-dev:i386
```

A missing 32-bit zlib shows up as `Could NOT find ZLIB (missing: ZLIB_LIBRARY)` at configure time.

Building a 32-bit zlib yourself and pointing `ZLIB_ROOT` at it works too, and a **static** one is
worth the trouble: it leaves `libc6:i386` as the server's only 32-bit runtime dependency, which
matters on a host where enabling multiarch is a nuisance.

```bash
CC=gcc CFLAGS="-m32 -fPIC -O3" ./configure --static --prefix=/opt/zlib-i386
make && make install
cmake --preset linux-gcc-release -DZLIB_ROOT=/opt/zlib-i386
```

`-fPIC` is not optional there - the server links as a position-independent executable.

GCC is what ships. A Clang build is a diagnostic second opinion and is much noisier - see `todo.md`.
A GCC Release build is the artifact to ship, straight out of `build/<preset>/kpded2` - there is no
separate stripped copy, because Release never generates debug info in the first place.

| configuration | flags |
|---|---|
| `Debug` | `-O0 -g` |
| `RelWithDebInfo` | `-O2 -g -DNDEBUG` |
| `Release` | `-O3 -DNDEBUG` |

Reach for `RelWithDebInfo` when a bug needs hunting: it is optimised, so it fails the way the
shipped build fails. Note it builds at **`-O2`**, which warns about things `-O3` does not.

> Changing anything in `cmake/toolchains/` requires deleting `build/` first. `CMAKE_C_FLAGS_INIT`
> only seeds the cache on a build tree's **first** configure, so an existing tree silently keeps the
> old flags.

## Building on Windows

`kpded2.sln` is the only proven Windows entry point. The `windows-msvc-*` presets exist but have
never been run - see `todo.md` before relying on them.
