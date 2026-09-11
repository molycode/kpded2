# TODO

Work that is understood but parked, because it cannot be finished or verified in the environment it
was found in. Each item states what is known, what is only predicted, and what would settle it.

## Clear the remaining warnings, then turn on -Werror

`cmake/compilers/{gcc,clang}.cmake` build with `-Wall -Wextra -Wno-unused-parameter` but **without**
`-Werror`, and `msvc.cmake` without `/WX`. That is a deviation from the house convention and it is
temporary: a GCC 16 Release build still emits 54 warnings, so turning them into errors today would
simply make the tree unbuildable.

All four Linux presets build with **0 errors**. Measured at `5c3aa5c`:

| preset | warnings |
|---|---|
| `linux-gcc-debug` | 57 |
| `linux-gcc-release` | 55 |
| `linux-clang-debug` | 1382 |
| `linux-clang-release` | 1382 |

What is left on GCC:

| count | warning | character |
|---|---|---|
| 33 | `-Wsign-compare` | mostly loop counters against `.intvalue` and sizes; each needs a look at whether the signed side can go negative |
| 10 | `-Wpointer-sign` | `char *` against `byte *` at the network and filesystem boundaries |
| 9 | `-Wunused-but-set-variable` | dead locals, but some are debug accounting that a `#ifdef` no longer compiles |
| 2 | `-Wunused-function` | `_password_changed` and `SV_RunPmoves`, both `static` and both unreferenced in this configuration |

None of these is known to be a bug. They are volume work that wants a careful pass per warning, not a
blanket cast. Once the count is zero, restore `-Werror` to both GCC and Clang and `/WX` to MSVC, and
this section goes away.

Debug adds 3 `-Wformat-overflow` that Release does not.

**Clang's 1382 is really 55.** 1327 of them are a single repeated `-Wunknown-attributes`: Clang does
not implement `callee_pop_aggregate_return` and ignores it. Guarding the attribute collapses Clang to
GCC's number, and is the cheapest thing to do first:

```c
#if defined(__GNUC__) && !defined(__clang__)
#define EXPORT __attribute__((callee_pop_aggregate_return(1)))
#else
#define EXPORT
#endif
```

Ignoring the attribute is **not** a miscompile - the Clang build's `SV_Trace` still ends `ret $0x4`,
because Clang's i386 default is already callee-pops (verified with objdump). Keep the attribute for
GCC, where the original `(0)` was actively harmful; on Clang it only ever was a no-op.

**Clang finds one thing GCC does not:** `-Wsometimes-uninitialized` at `qcommon/cmodel.c:657`,
"variable 'p' is used uninitialized whenever '||' condition is true". That is the only remaining
warning in the class that has actually produced bugs in this tree - look at it first.

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
