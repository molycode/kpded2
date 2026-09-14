add_library(KpdCompileFlags INTERFACE)
target_compile_options(KpdCompileFlags INTERFACE
	-Wall
	-Wextra
	-Werror
	-Wno-unused-parameter
)

message(STATUS "Clang ${CMAKE_C_COMPILER_VERSION} compiler flags configured")
