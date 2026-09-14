# Selects the compiler flag set for first-party code, creating the KpdCompileFlags INTERFACE library.
# Its own file so the selection stays in one place, mirroring the kp-mod module layout.

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	include(${CMAKE_CURRENT_LIST_DIR}/compilers/msvc.cmake)
elseif(CMAKE_C_COMPILER_ID MATCHES "[Cc]lang")
	include(${CMAKE_CURRENT_LIST_DIR}/compilers/clang.cmake)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	include(${CMAKE_CURRENT_LIST_DIR}/compilers/gcc.cmake)
endif()

# One option rather than raw flags in a preset: the toolchain owns CMAKE_C_FLAGS_INIT
# (-m32 -mstackrealign, and -mstackrealign is mandatory here), and a preset that sets CMAKE_C_FLAGS
# replaces it instead of adding to it.
set(KPD_SANITIZER "none" CACHE STRING "Sanitizer to build with: none, address or undefined")
set_property(CACHE KPD_SANITIZER PROPERTY STRINGS none address undefined)

if(NOT KPD_SANITIZER STREQUAL "none")
	if(NOT KPD_SANITIZER MATCHES "^(address|undefined)$")
		message(FATAL_ERROR "KPD_SANITIZER is '${KPD_SANITIZER}'; expected none, address or undefined")
	endif()

	if(MSVC)
		message(FATAL_ERROR "KPD_SANITIZER has never been exercised with MSVC; wire it before using it")
	endif()

	# Instrumentation defeats GCC's value-range propagation, so it reports objects as possibly at
	# address zero and loses track of the terminator in Q_strncpy. Every instance checked is a false
	# positive, and these three stay enabled in the configurations that actually ship.
	target_compile_options(KpdCompileFlags INTERFACE -fsanitize=${KPD_SANITIZER} -fno-omit-frame-pointer
		$<$<C_COMPILER_ID:GNU>:-Wno-stringop-truncation>
		$<$<C_COMPILER_ID:GNU>:-Wno-stringop-overread>
		$<$<C_COMPILER_ID:GNU>:-Wno-format-overflow>)
	target_link_options(KpdCompileFlags INTERFACE -fsanitize=${KPD_SANITIZER})
	message(STATUS "Sanitizer enabled: ${KPD_SANITIZER}")
endif()
