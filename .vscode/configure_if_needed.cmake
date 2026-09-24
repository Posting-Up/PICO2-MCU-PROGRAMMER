# ------------------------------------------------------------------------------------------------
# configure_if_needed.cmake
#
# Run by the "Configure Project" task in tasks.json:
#     cmake -DPICO_PROJECT_DIR=<dir> -DCMAKE_EXE=<cmake> -DNINJA_EXE=<ninja> -P configure_if_needed.cmake
#
# Generates <dir>/build (Ninja, Release) when build/build.ninja does not exist yet, and
# does nothing otherwise (Ninja re-runs CMake by itself whenever CMakeLists.txt changes).
# ------------------------------------------------------------------------------------------------
if(EXISTS "${PICO_PROJECT_DIR}/build/build.ninja")
    message(STATUS "build/ already configured - skipping CMake configure")
    return()
endif()

message(STATUS "build/ not configured - running CMake configure")

execute_process(
    COMMAND "${CMAKE_EXE}"
            -S "${PICO_PROJECT_DIR}"
            -B "${PICO_PROJECT_DIR}/build"
            -G Ninja
            "-DCMAKE_MAKE_PROGRAM=${NINJA_EXE}"
            -DCMAKE_BUILD_TYPE=Release
    RESULT_VARIABLE CONFIGURE_RESULT
)

if(NOT CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "CMake configure failed (exit code ${CONFIGURE_RESULT})")
endif()
