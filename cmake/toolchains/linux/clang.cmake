set(CMAKE_SYSTEM_NAME Linux)

set(KPD_ARCH "i386" CACHE STRING "Target architecture: i386 or x86_64")
set_property(CACHE KPD_ARCH PROPERTY STRINGS i386 x86_64)

# Both paths accept a cache variable and fall back to the environment. An environment-only setting is
# unreachable from an IDE: QtCreator applies a preset's `environment` block to the BUILD step, never to
# the configure, so the toolchain would never see it.
if(NOT KPD_CLANG_PATH AND DEFINED ENV{KPD_CLANG_PATH})
	set(KPD_CLANG_PATH "$ENV{KPD_CLANG_PATH}" CACHE PATH "Clang installation root")
endif()

if(NOT KPD_GCC_PATH AND DEFINED ENV{KPD_GCC_PATH})
	set(KPD_GCC_PATH "$ENV{KPD_GCC_PATH}" CACHE PATH "GCC installation supplying the 32-bit runtime to Clang builds")
endif()

# try_compile re-runs this file in a scratch project that inherits the environment but NOT the cache, so
# without this the compiler-ABI probe would measure a different toolchain than the build then uses.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES KPD_CLANG_PATH KPD_GCC_PATH KPD_ARCH)

if(KPD_CLANG_PATH)
	set(CMAKE_C_COMPILER "${KPD_CLANG_PATH}/bin/clang" CACHE FILEPATH "" FORCE)
elseif(NOT DEFINED CMAKE_C_COMPILER)
	set(CMAKE_C_COMPILER "clang")
endif()

# The project is C-only, but an IDE's preset probe generates a project that enables CXX by default,
# so the toolchain has to describe a working C++ compiler too or that probe fails and the presets
# never appear.
if(KPD_CLANG_PATH)
	set(CMAKE_CXX_COMPILER "${KPD_CLANG_PATH}/bin/clang++" CACHE FILEPATH "" FORCE)
elseif(NOT DEFINED CMAKE_CXX_COMPILER)
	set(CMAKE_CXX_COMPILER "clang++")
endif()

# The server loads only a game library of its own width: i386 for the 1999 libraries and their
# builds, x86_64 for a 64-bit one.
if(KPD_ARCH STREQUAL "i386")
	set(CMAKE_SYSTEM_PROCESSOR i386)
	# -mstackrealign: a 1999 game library calls back into the server on a 4-byte-aligned stack, but the
	# compiler assumes the modern 16-byte ABI, so an aligned SSE spill (movapd) faults. Costs a prologue.
	set(_kpd_cflags "-m32 -mstackrealign")
	set(_kpd_mflag "-m32")
	# Debian multiarch: the 32-bit libraries live in a per-architecture directory, and find_library()
	# would otherwise see only the host's 64-bit ones and reject them as incompatible.
	set(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)
elseif(KPD_ARCH STREQUAL "x86_64")
	set(CMAKE_SYSTEM_PROCESSOR x86_64)
	set(_kpd_cflags "-m64")
	set(_kpd_mflag "-m64")
	set(CMAKE_LIBRARY_ARCHITECTURE x86_64-linux-gnu)
else()
	message(FATAL_ERROR "KPD_ARCH is '${KPD_ARCH}'; expected i386 or x86_64")
endif()

set(CMAKE_C_FLAGS_INIT "${_kpd_cflags}")
# The linker flags below are language-agnostic, so C++ must match the C target or it produces objects
# of the other width against the link.
set(CMAKE_CXX_FLAGS_INIT "${_kpd_cflags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_kpd_mflag}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_kpd_mflag}")

if(KPD_GCC_PATH)
	file(GLOB _gcc_ver_dirs LIST_DIRECTORIES true "${KPD_GCC_PATH}/lib/gcc/x86_64-pc-linux-gnu/*")
	list(SORT _gcc_ver_dirs COMPARE NATURAL ORDER DESCENDING)
	list(GET _gcc_ver_dirs 0 _gcc_install_dir)
	add_compile_options(--gcc-install-dir=${_gcc_install_dir})
	add_link_options(--gcc-install-dir=${_gcc_install_dir})
	message(STATUS "Using GCC runtime from KPD_GCC_PATH: ${KPD_GCC_PATH}")
else()
	message(STATUS "Using the system GCC runtime (set KPD_GCC_PATH to pin one)")
endif()

if(KPD_CLANG_PATH)
	message(STATUS "Using Clang from KPD_CLANG_PATH: ${KPD_CLANG_PATH}")
else()
	message(STATUS "Using system Clang (set KPD_CLANG_PATH to use custom Clang)")
endif()

message(STATUS "Target architecture: ${KPD_ARCH}")
message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
