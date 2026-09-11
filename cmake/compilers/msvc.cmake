add_library(KpdCompileFlags INTERFACE)
target_compile_options(KpdCompileFlags INTERFACE
	/W4
	/wd4100
	/permissive-
)

message(STATUS "MSVC ${CMAKE_C_COMPILER_VERSION} compiler flags configured")
