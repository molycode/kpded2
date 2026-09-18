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

## Done 2026-09-18: the message of the day

Closed, and it did not land here. A player typed `motd` on 2026-09-16, got nothing and left; the
word arrived as chat because kp-mod's `ClientCommand` ends in `Cmd_Say_f`.

Two halves, both now settled:

- **Connect-time.** kpded2 already inherits R1Q2's `sv_connectmessage` (`sv_main.c:3714`, sent at
  `:1774`). It only ever defaulted to `""`, so this was a `server.cfg` line and no code. Set on both
  live servers.
- **On demand.** Built in **kp-mod**, not here. A `motd` command was written in this tree first and
  then reverted: `ucmds[]` is walked *before* the game library and `return`s on a hit, so an engine
  command would permanently shadow any MOTD a game library implements - including ours.

**Measured, and the reason it cannot live here:** the coloured text in Kingpin's screens comes from
the layout language (`dmstr <rgb>`, three digits), and the client only draws a layout while the
*game* sets `stats[STAT_LAYOUTS]`. An engine-sent layout draws nothing - probed against the retail
client. A `svc_centerprint` does work from the engine, but only in white; setting bit 7 on the text
selects the alternate charset, which Kingpin renders as bold rather than a second colour.

Two engine behaviours worth remembering, both found here:

- **`SV_AddMessageSingle` drops duplicate layouts** - it caches `cl->layout` per client and discards
  a byte-identical resend. A static screen therefore cannot be "refreshed"; the bagman team dialog
  only survives its periodic re-send because its contents change every time.
- **`+set` on the command line strips high-bit bytes**, while rcon and `server.cfg` preserve them.

## Done 2026-09-14: the smaller items flagged during triage

All cleared, one commit each:

- **`sv_user.c`** -- three sites scaled a client-supplied number by 1366 or shifted it left ten
  places before range-checking it, so the arithmetic overflowed on a network-driven path and the
  wrapped value could still pass the check that followed. Each now ranges the count first.
- **`cvar.c`** -- the hostname start-time scan declared `int` and passed the addresses to `%u`.
- **`sv_ccmds.c`** -- `qCopyFile` ignored `fwrite`'s return, so a full disk produced a silently
  truncated savegame copy while the read side checked `ferror`.

**92 is the current baseline.**

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
