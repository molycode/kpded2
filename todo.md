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

**THE CLANG-TIDY TRIAGE IS COMPLETE, 2026-09-14.** Every check was triaged; the exclusions in
`.clang-tidy` each carry the reason. A tree-wide run is now **95 findings over 16 checks**, all
individually examined and recorded false. Most are the `gi.error`/`Com_Error`-is-noreturn artifact:
the analyzer cannot see the attribute through a function pointer, so every guard built on it reads
as falling through.

Notable: of the 29 analyzer memory-safety findings, **all 29 were false positives** -- every defect
fixed in this tree came from reading around the findings rather than from a finding itself.

## Open: flagged while triaging, each a separate change

- **`server/sv_user.c:2459`** -- `VarBanMatch` converts a client's cvar reply with `atof`, so a
  non-numeric reply compares equal to 0. A `=0` cvarban then matches innocent clients (and
  `CVARBAN_BLACKHOLE` blackholes their IP), while `>N`/`<N` rules are evaded by replying with any
  non-numeric string. The empty-string half is deliberate -- MH commented out the guard and annotated
  it "empty string = 0" -- but the non-numeric half is a different case. Fixing it is a policy
  decision about what a numeric ban operator should do with unparseable input.
- **`server/sv_user.c:1642, 1647, 1799`** -- `atoi(...) * 1366` and `atoi(...) << 10` are computed
  *before* the range test, so a client sending a huge number causes signed overflow. The wrapped
  value is still range-checked, so there is no out-of-bounds access, but the overflow is UB on a
  network-driven path.
- **`server/sv_ccmds.c:84`** -- `char num[128]` filled by an unbounded `num[j++] = *s++`; `sv addip`
  with 128+ digits smashes the stack. Console/rcon only.
- **`qcommon/cvar.c:461`** -- `int h, m;` passed to `sscanf` `%u`. Harmless today because the string
  is re-parsed and range-checked as unsigned, but it is a type mismatch.
- **`server/sv_ccmds.c:344`** -- `qCopyFile` ignores `fwrite`'s return, so a full disk silently
  truncates a savegame copy while the read side does check `ferror`.

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
