# TODO

Work that is understood but parked, because it cannot be finished or verified in the environment it
was found in. Each item states what is known, what is only predicted, and what would settle it.

## Turn on /WX for MSVC

`cmake/compilers/msvc.cmake` builds with `/W4 /wd4100` but without `/WX`, while GCC and Clang carry
`-Werror`. That is the one remaining deviation from the kp-mod convention, and it is deliberate:
**the MSVC build has never been configured or compiled here**, see below, so turning `/WX` on blind
would hand the next Windows session a tree that may not build.

**What would settle it:** build on Windows, clear whatever `/W4` reports, then add `/WX` - directly
after `/W4`, which is where kp-mod's own `msvc.cmake` carries it.

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
