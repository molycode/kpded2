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

## Keep clang-tidy at its baseline

The triage is complete: every finding was examined, the real ones were fixed in their own commits,
and each exclusion in `.clang-tidy` carries the reason it was measured to be noise. **92 is the
current baseline**, counted as unique `realpath:line:col`. A run above it has found something new;
triage only the new findings. Re-run after any substantial change.

Most of the residue is the `gi.error`/`Com_Error`-is-noreturn artifact: the analyzer cannot see the
attribute through a function pointer, so every guard built on it reads as falling through. Of the
29 analyzer memory-safety findings, all 29 were false; every defect fixed in this tree came from
reading around the findings rather than from a finding itself.

Run it with the pinned Clang (`$KPD_CLANG_PATH`, the root your user presets pass), against the
compile database that `cmake --preset linux-clang_23-relwithdebinfo` writes:

    $KPD_CLANG_PATH/bin/clang-tidy -p build/clang_23-RelWithDebInfo server/<file>.c

Tree-wide, which is the only way the header findings deduplicate:

    $KPD_CLANG_PATH/bin/run-clang-tidy -clang-tidy-binary $KPD_CLANG_PATH/bin/clang-tidy \
      -p build/clang_23-RelWithDebInfo -quiet -j 8 '/(qcommon|server|game|linux)/'

Neither binary is on `PATH`, and `run-clang-tidy` needs `-clang-tidy-binary` even when called by its
full path. When counting findings, resolve each path with `realpath` first: the same header arrives
under several spellings (`qcommon/../game/q_shared.h` and so on), so a naive key inflates the count
several-fold.

Disable a check only once its findings are shown to be false, with the reason written into
`.clang-tidy`.

## List game-made players in the server browsers

A game library can put players into client slots itself, without the engine connecting anything
there: a co-op companion-bot game library calls `ClientConnect` and `ClientBegin` on a free slot.
Both places that report players only walk `svs.clients` at `cs_connected` or later -
`SV_StatusString` (status replies and master heartbeats) and the GameSpy reply in `sv_main.c`
(`numplayers` and `player_<n>`) - so those players never show. Measured 2026-10-09 on a local
server: one human and seven bots in the game, one player in the status reply. A server with bots
looks emptier than it is.

Known: such a slot's edict is `inuse` with a `client`, while its `client_t` stays `cs_free`; the game
sets `CS_PLAYERSKINS + slot` (`name\model/skin`) for it as for any player, so the name and
`ps.stats[STAT_FRAGS]` are at hand without a new game interface.

Wanted: list them by name, so a server looks as busy as it is, but never as full - at most
`maxclients - 1` players reported (counting `sv_reserved_slots`, which already lowers the advertised
`maxclients`), so a browser filter that hides full servers still shows it.

Open: whether to mark them (ping 0, a tag) - unmarked looks busier but misleads someone looking for
humans; whether `sv_validate_playerskins` passes a game-made player's configstring; the public
servers' status page reads the status reply and should then agree.

Settled by: a local server with game-made players whose status reply, heartbeat and GameSpy reply
list them by name and stop one short of the advertised `maxclients`, seen in a stock client's
server browser.
