add_library(KpdCompileFlags INTERFACE)
target_compile_options(KpdCompileFlags INTERFACE
	-g
	-Wall
	-Wextra
	-Wno-unused-parameter
	-Werror
)

message(STATUS "Clang ${CMAKE_C_COMPILER_VERSION} compiler flags configured")
