set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i386)

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
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES KPD_CLANG_PATH KPD_GCC_PATH)

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

# The game library this server dlopens is i386, so the server must be too.
# -mstackrealign: a 1999 game library calls back into the server on a 4-byte-aligned stack, but the
# compiler assumes the modern 16-byte ABI, so an aligned SSE spill (movapd) faults. Costs a prologue.
set(CMAKE_C_FLAGS_INIT "-m32 -mstackrealign")
# The linker flags below are language-agnostic, so C++ must be built 32-bit as well or it produces
# a 64-bit object against a 32-bit link.
set(CMAKE_CXX_FLAGS_INIT "-m32 -mstackrealign")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-m32")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-m32")

# Debian multiarch: the 32-bit libraries live in a per-architecture directory, and find_library()
# would otherwise see only the host's 64-bit ones and reject them as incompatible.
set(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)

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

message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
