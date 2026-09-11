# Run in script mode from a build-time custom target, so the recorded revision cannot go stale the
# way a configure-time value would. Writes through copy_if_different: an unchanged revision must not
# force a rebuild of everything that includes the header.
find_package(Git QUIET)

set(KPD_REV "unknown")

if(GIT_EXECUTABLE)
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
		WORKING_DIRECTORY "${SRC_DIR}"
		OUTPUT_VARIABLE KPD_REV
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
		RESULT_VARIABLE KPD_REV_RESULT)

	if(NOT KPD_REV_RESULT EQUAL 0 OR KPD_REV STREQUAL "")
		set(KPD_REV "unknown")
	else()
		# A "+" marks uncommitted changes, so a binary built from an edited tree can never be
		# mistaken for the commit it names.
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" diff --quiet HEAD
			WORKING_DIRECTORY "${SRC_DIR}"
			RESULT_VARIABLE KPD_DIRTY
			ERROR_QUIET)
		if(NOT KPD_DIRTY EQUAL 0)
			set(KPD_REV "${KPD_REV}+")
		endif()
	endif()
endif()

file(WRITE "${OUT_FILE}.tmp" "#define KPD_GIT_REV \"${KPD_REV}\"\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${OUT_FILE}.tmp" "${OUT_FILE}")
file(REMOVE "${OUT_FILE}.tmp")
