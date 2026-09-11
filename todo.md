# TODO

Work that is understood but parked, because it cannot be finished or verified in the environment it
was found in. Each item states what is known, what is only predicted, and what would settle it.

## Turn on -Werror

**The tree is warning-free.** A clean build of every Linux preset - `linux-gcc_16-{debug,release}`
and `linux-clang_22-{debug,release}` - reports 0 errors and 0 warnings. The one line left in the GCC
Release log is `lto-wrapper: warning: using serial compilation of 4 LTRANS jobs`, which is LTO
telling you about its job count, not a diagnostic.

That clears the condition this section used to describe. What remains is the switch itself:
`-Werror` in `cmake/compilers/{gcc,clang}.cmake` and `/WX` in `msvc.cmake`, all three left off
deliberately while the count came down from 57/55/1382. **`/WX` cannot be verified here** - the MSVC
build is unbuilt (see below), and turning it on blind would hand the next Windows session a tree that
may not compile. Either turn both on and treat the first Windows build as the test, or turn on
`-Werror` now and `/WX` in the Windows session.

What the cleanup found, all fixed and each in its own commit: an alias-expansion stack overflow, an
rcon redirect that ran past `sv_outputbuf`, an unvalidated MDX header, two packetdup cvars never
floored at zero, a dropped `state &&` guard in `Com_Error`'s auto-restart, and a stack smash from a
long filename in the game directory. Four of those were reproduced against a running server.

**The rule the passes established: never cast or delete to silence a warning.** In `SV_PacketDup_f`
the implicit unsigned conversion *is* the check that rejects a client's `packetdup -1`; casting the
other way would have introduced the bug it prevents. In `Com_Error` the unused variable was the
surviving half of a guard somebody deleted by accident. Read what the warning is pointing at before
deciding it is noise.

## killserver crashes the dedicated server

Not found by a warning, and not fixed - noticed while testing the above, and present on `3117d48`
as well, so it is not something this work introduced.

`killserver` on its own segfaults. Minimal repro, from a staged game directory:

```
echo killserver > main/kill.cfg
./kpded2 +set dedicated 1 +set public 0 +map kpdm1 +exec kill.cfg
```

The core lands in `SV_SpawnServer` at `server/sv_init.c:559`, `svs.clients[i].state == cs_spawned`,
reached from `SV_Map` <- `SV_GameMap_f` <- `SV_Map_f` <- `Cbuf_Execute`.

**What it looks like:** `svs.clients` is allocated in exactly one place, `SV_InitGame`
(`sv_init.c:828`), and `SV_Shutdown` releases it. `SV_Map` only calls `SV_InitGame` again when
`sv.state == ss_dead`, so if `killserver` leaves `sv.state` set to anything else the next `map`
walks a freed `svs.clients`. **What would settle it:** check what `SV_Shutdown` does to `sv.state`
and to `svs.clients` against what `SV_Map` tests, on a debug build where the pointer can be read
back after the shutdown.

## q2ded2 is not in the CMake build

`Makefile` builds two targets, `kpded2` and `q2ded2`; the CMake build defines only `Kpded2`. This is
a deliberate divergence, not an oversight.

`q2ded2` compiles `qcommon/unzip.c`, which is vendored minizip: its crypt helpers take
`const unsigned long *pcrc_32_tab`, while zlib 1.2.7 and later return `const z_crc_t *` (that is,
`const unsigned int *`) from `get_crc_table()`. GCC 14 and later make the assignment a hard error
rather than a warning:

```
qcommon/unzip.c:1184:24: error: assignment to 'const long unsigned int *' from incompatible
pointer type 'const z_crc_t *'
```

Repairing it means changing the signatures in `qcommon/crypt.h` and the struct member that feeds
them - vendored third-party code, in a target this fork does not use. The `Makefile` still builds
`q2ded2` with the system GCC 13, which only warns.

**What would settle it:** refresh the vendored minizip to a version that uses `z_crc_t`, or decide
that a Kingpin fork has no business shipping a plain Quake 2 server and drop the target from the
`Makefile` too.

## Windows is unbuilt and untested

The Windows branch of `CMakeLists.txt` mirrors the file list in `kpded2.vcxproj` and the
`windows-msvc-*` presets use the Visual Studio generator with `"architecture": {"value": "Win32"}`,
so they should produce a 32-bit binary. **None of it has been configured or compiled**, because
there is no Windows machine here.

Predicted, not verified:

1. `qcommon/crc.c`, `ioapi.c` and `unzip.c` compile on Windows against the vendored `zlib/zlib.h`,
   which is old enough to still use `unsigned long` - this is why the Windows build takes them and
   Linux does not.
2. `set_source_files_properties(... COMPILE_OPTIONS "/w")` reaches those three files. It is set from
   the top-level `CMakeLists.txt` with an explicit `DIRECTORY`, which should be correct, but
   source-file properties are directory-scoped and easy to get wrong.
3. The link resolves. **`kpded2.vcxproj` links `msvcrt.lib` and `msvcrt_winxp.obj`**, checked into
   the repository root, to keep the binary running on Windows XP. The CMake build reproduces neither.
   If XP support still matters, that has to be carried across before a CMake-built Windows binary
   replaces the `.vcxproj` one.

Until then `kpded2.sln` remains the only proven Windows entry point.

## The i386 aggregate-return fix should go upstream

`game/q_shared.h` had `callee_pop_aggregate_return(0)` under `#if KINGPIN`, commented as matching
GCC 2.7.2. It is the reverse of what the 1999 binaries do, and it corrupts the stack of any game
library that calls a struct-returning import such as `gi.trace()`. The measurements are in the commit
message. This breaks every Linux kpded2 build against every Kingpin game library, retail included -
it is worth offering to MonkeyHarris as a pull request rather than carrying privately.
