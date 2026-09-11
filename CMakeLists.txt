# kpded2 - MonkeyHarris's Kingpin server, built on r1ch's R1Q2 Quake 2 server.
cmake_minimum_required(VERSION 4.3.2)

project(Kpded2 VERSION 2.2.0 LANGUAGES C)

if(PROJECT_IS_TOP_LEVEL)
	set(CMAKE_C_STANDARD 11)
	set(CMAKE_C_STANDARD_REQUIRED ON)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)

	# Release takes CMake's -O3 -DNDEBUG as-is. Debug's default -g is cleared because
	# cmake/compilers/ already adds it for both configurations, and Release needs it: the stripped
	# ship/ copy only symbolises against the unstripped binary beside it.
	set(CMAKE_C_FLAGS_DEBUG "")
endif()

if(NOT COMMAND KpdApplyCompileFlags)
	include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/kpd_targets.cmake)
endif()

if(NOT TARGET KpdCompileFlags)
	include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compile_flags.cmake)
endif()

# Listed rather than globbed: the file set is curated, not "every .c here". qcommon/net_common.c
# exists but is deliberately not built, and the zip sources are a Windows-only addition.
set(KPD_SOURCES
	game/q_shared.c
	qcommon/cmd.c
	qcommon/cmodel.c
	qcommon/common.c
	qcommon/cvar.c
	qcommon/files.c
	qcommon/md4.c
	qcommon/mersennetwister.c
	qcommon/net_chan.c
	qcommon/pmove.c
	qcommon/redblack.c
	server/sv_ccmds.c
	server/sv_ents.c
	server/sv_game.c
	server/sv_init.c
	server/sv_main.c
	server/sv_send.c
	server/sv_user.c
	server/sv_world.c
)

# MaxMind's libmaxminddb, vendored. Its own target because upstream compiles it as strict C99 while
# the rest of the tree is built with GNU extensions.
add_library(KpdGeoIp OBJECT geoip/data-pool.c geoip/maxminddb.c)
set_target_properties(KpdGeoIp PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_include_directories(KpdGeoIp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/geoip)
KpdApplyCompileFlags(KpdGeoIp)
KpdSuppressExternalWarnings(KpdGeoIp)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	list(APPEND KPD_SOURCES
		win32/conproc.c
		win32/net_wins.c
		win32/q_shwin.c
		win32/sys_win.c
		qcommon/crc.c
		qcommon/ioapi.c
		qcommon/unzip.c
	)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND KPD_SOURCES
		linux/glob.c
		linux/net_udp.c
		linux/q_shlinux.c
		linux/sys_linux.c
	)
else()
	message(FATAL_ERROR "Unsupported platform '${CMAKE_SYSTEM_NAME}'. Add it to CMakeLists.txt.")
endif()

add_executable(Kpded2 ${KPD_SOURCES})
set_target_properties(Kpded2 PROPERTIES OUTPUT_NAME "kpded2")
target_include_directories(Kpded2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Kpded2 PRIVATE KpdGeoIp)
target_compile_definitions(Kpded2 PRIVATE DEDICATED_ONLY KINGPIN)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions(Kpded2 PRIVATE LINUX)

	# Found rather than named on the link line, so a missing 32-bit zlib is a configure error that
	# says "ZLIB" instead of a linker one that says "cannot find -lz".
	find_package(ZLIB REQUIRED)
	target_link_libraries(Kpded2 PRIVATE m ZLIB::ZLIB pthread dl rt)

	# The game library is dlopened and resolves floor() and friends from the global scope, so libm has
	# to be a real DT_NEEDED. Ubuntu links --as-needed by default and would drop it.
	target_link_options(Kpded2 PRIVATE "-Wl,--no-as-needed")
	target_link_options(Kpded2 PRIVATE "-Wl,--no-undefined" "-Wl,--gc-sections")

	# The stripped ship/ copy keeps this id, so a crash there symbolises against the build beside it.
	target_link_options(Kpded2 PRIVATE "-Wl,--build-id=sha1")
endif()

if(NOT MSVC)
	# 1999 inline semantics: a bare `inline` must still emit an external definition, or
	# SV_FixPlayerSkin has no out-of-line copy for PF_Configstring to call.
	target_compile_options(Kpded2 PRIVATE -fgnu89-inline)

	# Upstream's Makefile flags. -fno-associative-math keeps float results close enough to the
	# client's for prediction; -fno-finite-math-only avoids pulling in a GLIBC_2.15 symbol.
	target_compile_options(Kpded2 PRIVATE -ffast-math -fno-associative-math -fno-finite-math-only)

	# Quake 2 type-puns floats through int pointers throughout. The reads are real, so the honest fix is
	# to stop promising the optimiser they cannot alias rather than to silence the 149 warnings it emits.
	target_compile_options(Kpded2 PRIVATE -fno-strict-aliasing)

	# GCC runs LTRANS serially unless given a job count, and Clang has no such spelling.
	if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
		set(KPD_LTO_FLAG -flto=auto)
	else()
		set(KPD_LTO_FLAG -flto)
	endif()

	target_compile_options(Kpded2 PRIVATE $<$<CONFIG:Release>:${KPD_LTO_FLAG}>)
	target_link_options(Kpded2 PRIVATE $<$<CONFIG:Release>:${KPD_LTO_FLAG}>)
endif()

# Stamp the git revision into the binary. A custom target rather than a configure-time variable, so
# a build after a new commit records that commit instead of whatever configure last saw.
set(KPD_VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kpd_version.h")
add_custom_target(KpdVersion ALL
	COMMAND ${CMAKE_COMMAND}
		-DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-DOUT_FILE=${KPD_VERSION_HEADER}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/kpd_version.cmake
	BYPRODUCTS ${KPD_VERSION_HEADER}
	COMMENT "Recording the git revision")
add_dependencies(Kpded2 KpdVersion)
target_include_directories(Kpded2 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

KpdApplyCompileFlags(Kpded2)

# minizip and zlib's crc, vendored into qcommon/ rather than kept apart, so the suppression has to be
# per source file. Linux does not compile them at all.
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_source_files_properties(qcommon/crc.c qcommon/ioapi.c qcommon/unzip.c
		DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		PROPERTIES COMPILE_OPTIONS "/w")
endif()

# Only GCC produces a ship/ artifact: a Clang build is a diagnostic second opinion, never the
# released server. The stripped copy keeps the built one's build id, so a crash in the shipped binary
# symbolises against the symbols left beside it.
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
	set(KPD_SHIP_DIR "$<TARGET_FILE_DIR:Kpded2>/ship")
	add_custom_command(TARGET Kpded2 POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${KPD_SHIP_DIR}"
		COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Kpded2>" "${KPD_SHIP_DIR}/$<TARGET_FILE_NAME:Kpded2>"
		COMMAND ${CMAKE_STRIP} --strip-all "${KPD_SHIP_DIR}/$<TARGET_FILE_NAME:Kpded2>"
		COMMENT "Stripping the shipping server into ship/"
		VERBATIM)
endif()
