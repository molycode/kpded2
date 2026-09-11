set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i386)

# Cache variable first, environment as a fallback: a preset's `environment` block never reaches an IDE's
# configure step, so an env-only setting is silently lost there.
if(NOT KPD_GCC_PATH AND DEFINED ENV{KPD_GCC_PATH})
	set(KPD_GCC_PATH "$ENV{KPD_GCC_PATH}" CACHE PATH "GCC installation root")
endif()

# try_compile re-runs this file in a scratch project that inherits the environment but NOT the cache, so the
# compiler-ABI probe would otherwise fall back to the system GCC and measure the wrong toolchain.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES KPD_GCC_PATH)

if(NOT DEFINED CMAKE_C_COMPILER)
	if(KPD_GCC_PATH)
		set(CMAKE_C_COMPILER "${KPD_GCC_PATH}/bin/gcc")
	else()
		set(CMAKE_C_COMPILER "gcc")
	endif()
endif()

# The project is C-only, but an IDE's preset probe generates a project that enables CXX by default,
# so the toolchain has to describe a working C++ compiler too or that probe fails and the presets
# never appear.
if(NOT DEFINED CMAKE_CXX_COMPILER)
	if(KPD_GCC_PATH)
		set(CMAKE_CXX_COMPILER "${KPD_GCC_PATH}/bin/g++")
	else()
		set(CMAKE_CXX_COMPILER "g++")
	endif()
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
	message(STATUS "Using GCC from KPD_GCC_PATH: ${KPD_GCC_PATH}")
else()
	message(STATUS "Using system GCC (set KPD_GCC_PATH to use custom GCC)")
endif()

message(STATUS "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
