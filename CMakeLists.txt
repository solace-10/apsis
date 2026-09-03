# 3.24 for $<LINK_LIBRARY:WHOLE_ARCHIVE>, which game/CMakeLists.txt needs to keep translation units
# that exist only for their static initialisers - see the comment on the game target there.
cmake_minimum_required(VERSION 3.24)
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standardq

# Build configuration defines
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(BUILD_DEBUG)
else()
    add_compile_definitions(BUILD_RELEASE)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pandora/cmake/shared)

# Enable compile commands for clangd, which is used by Cursor.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the minimum CMake version for upstream projects, as some reference
# older versions which are no longer supported by modern CMake.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(EMSCRIPTEN)
    add_custom_target(setup_compile_commands ALL)
else()
    add_custom_target(setup_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/compile_commands.json"
                "${CMAKE_SOURCE_DIR}/compile_commands.json"
        DEPENDS "${CMAKE_BINARY_DIR}/compile_commands.json"
        COMMENT "Copying compile_commands.json for clangd integration..."
    )
endif()

add_subdirectory("pandora")

# The presets configure with -Wno-dev/-Wno-deprecated so that pandora's vendored dependencies stop
# burying anything actionable; this puts the warnings back for the code we own. It has to sit
# between the two add_subdirectory calls - see pandora/cmake/shared/Diagnostics.cmake. game/tests,
# added at the bottom, inherits this state.
pandora_enable_project_diagnostics()

add_subdirectory("game")

# Tests are native-only: they cover logic that is platform-independent, so running them through
# emscripten would cost a node harness and buy nothing. The `tests` target is configured but is not
# in the build presets' target lists, so a normal build doesn't pay for it - run ./scripts/test.sh.
option(APSIS_BUILD_TESTS "Build the test suite (native only)." ON)
if(TARGET_PLATFORM_NATIVE AND APSIS_BUILD_TESTS)
    enable_testing()
    add_subdirectory("game/tests")
endif()
