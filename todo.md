# TODO

Open work. Each item states what is known, what is only predicted, and what would settle it.

## Build and verify the Windows path

The Windows branch of `CMakeLists.txt` mirrors the file list in `kpded2.vcxproj`, and the
`windows-msvc-*` presets use the Visual Studio generator with `"architecture": {"value": "Win32"}`.
**None of it has ever been configured or compiled** - there is no Windows machine here - so
`kpded2.sln` remains the only proven Windows entry point.

Predicted, not verified:

1. `qcommon/crc.c`, `ioapi.c` and `unzip.c` compile against the vendored `zlib/zlib.h`, which is old
   enough to still use `unsigned long`. This is why the Windows build takes them and Linux does not.
2. `set_source_files_properties(... COMPILE_OPTIONS "/w")` reaches those three files. It is set from
   the top-level `CMakeLists.txt` with an explicit `DIRECTORY`, which should be correct, but
   source-file properties are directory-scoped and easy to get wrong.
3. The link resolves. `kpded2.vcxproj` links `msvcrt.lib` and `msvcrt_winxp.obj`, both checked in at
   the repository root, to keep the binary running on Windows XP. The CMake build reproduces
   neither, so that has to be carried across first if XP support still matters.

`kpded2.vcxproj` is also still its own flag set - `MaxSpeed`, `WholeProgramOptimization`,
`MultiThreadedDLL` - independent of `cmake/compilers/msvc.cmake`. It is the last place CMake is not
the single source of truth for flags.

## Turn on /WX for MSVC

`cmake/compilers/msvc.cmake` builds with `/W4 /wd4100` but no `/WX`, while GCC and Clang carry
`-Werror`. Turning it on unverified would hand the next Windows session a tree that may not build.

**What would settle it:** build on Windows, clear whatever `/W4` reports, then add `/WX` directly
after `/W4`, where kp-mod's `msvc.cmake` carries it.

## Decide whether q2ded2 stays

`q2ded2` has no Linux build and never had a CMake target; it survives only as `q2ded2.sln`,
`q2ded2.vcxproj` and its `.filters`. It compiles `qcommon/unzip.c`, vendored minizip, whose crypt
helpers take `const unsigned long *pcrc_32_tab` while zlib 1.2.7 and later return `const z_crc_t *`
from `get_crc_table()` - a hard error on GCC 14 and later:

```
qcommon/unzip.c:1184:24: error: assignment to 'const long unsigned int *' from incompatible
pointer type 'const z_crc_t *'
```

The Windows `kpded2` build compiles those same files against the old vendored zlib header and
silences them with `/w`, so it is unaffected either way.

**What would settle it:** decide whether a Kingpin fork has any business shipping a plain Quake 2
server. If not, delete the three `q2ded2.*` files. If so, the vendored minizip needs refreshing to a
version using `z_crc_t` before it builds with a current compiler.

## Offer the i386 aggregate-return fix upstream

`game/q_shared.h` carried `callee_pop_aggregate_return(0)`, which is the reverse of what the 1999
binaries do and corrupts the stack of any game library calling a struct-returning import such as
`gi.trace()`. It breaks every Linux kpded2 build against every Kingpin game library, retail
included, so it is worth a pull request to MonkeyHarris rather than carrying privately. The
measurements are in the commit that fixed it.

**What would settle it:** Thomas's call on whether to open the PR.

## Triage the clang-tidy findings

`.clang-tidy` is in place and tuned for this tree, but nothing has been triaged yet. Every exclusion
in it is a check measured firing in the dozens or hundreds on code that is correct as written, and
each carries its reason in the file.

`bugprone-macro-parentheses` is DONE and the check is clean. **The memory-safety class is DONE too**
(2026-09-14): 29 findings triaged, 28 of them false positives. The three real defects fixed came out
of reading around the findings rather than from the findings themselves -- the Z_Realloc zone chain,
FS_LoadFile's pak handle, and FS_ListFiles' two sweeps. A tree-wide run is now **168 unique findings
over 24 checks**. What is left:

- `bugprone-unchecked-string-to-number-conversion`, 21. `atoi` on cvar and network input. Quake 2
  leans on atoi returning 0 for a bad value, so most of these are likely correct by design - but that
  has to be shown rather than assumed, which is why the check is not pre-excluded.
- Single findings worth reading first, because they are the kind the hand audits were hunting:
  `clang-analyzer-unix.Malloc`, `bugprone-suspicious-realloc-usage`, `clang-analyzer-core.DivideZero`,
  `clang-analyzer-deadcode.DeadStores`, `bugprone-suspicious-string-compare`.

Run it with the pinned Clang, against a compile database:

    /media/thomas/data/compilers/clang_22/bin/clang-tidy -p build/clang_22-RelWithDebInfo server/<file>.c

Tree-wide, which is the only way the header findings deduplicate:

    run-clang-tidy -clang-tidy-binary /media/thomas/data/compilers/clang_22/bin/clang-tidy \
      -p build/clang_22-RelWithDebInfo -quiet -j 8 '/(qcommon|server|game|linux)/'

`-clang-tidy-binary` is required - `run-clang-tidy` looks on PATH and in the build dir, and finds
neither. When counting findings, resolve each path with `realpath` first: the same header arrives
under several spellings (`qcommon/../game/q_shared.h` and so on), so a naive key inflates the count
several-fold.

Treat it like the -Wsign-compare pass: triage every finding into real or false, fix the real ones in
their own commits, and disable a check only once its findings are shown to be false, with the reason
written into `.clang-tidy`. That pass found four real bugs; the -O2 and sanitizer configurations added
two more. This is the next net of the same kind.
