project(tests)
set(CMAKE_CXX_STANDARD 20)

# Catch2 is fetched into the build directory rather than being vendored into a gitignored ext/
# like pandora's dependencies are. It is only ever needed by this target, and keeping it out of
# the source tree means a fresh worktree needs nothing set up for the tests to build.
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_PROGRESS TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# The suite links the whole game, as a library, rather than naming individual sources under test.
#
# That means the tests can reach code which touches components and scenes - MakeSGP4Elements()
# takes an OrbitalElementsComponent, and the orbital propagation is the most correctness-critical
# thing in the project - instead of being confined to whatever has already been extracted into
# engine-free translation units. The alternative costs a hand-maintained source list which silently
# defines what is testable, and it is the wrong constraint for this codebase.
#
# The price is that `tests` depends on pandora, so a clean worktree has to build the engine before it
# can run the suite. That is a cost you pay anyway to run the game. It is not paid per test run:
# libpandora.a and Dawn are static archives, so a test binary only drags in the objects whose
# symbols it actually references.
# Vallado's SGP4, vendored unmodified under reference/sgp4 - see the README beside it. Its
# own target, so that it is never linked into game_lib and so never reaches a shipped binary,
# and so that its warnings and its formatting stay off this project's books.
add_library(sgp4_reference STATIC reference/sgp4/SGP4.cpp)
target_include_directories(sgp4_reference SYSTEM PUBLIC reference/sgp4)
target_compile_options(sgp4_reference PRIVATE -w)

add_executable(tests
    components/orbital_elements_component_tests.cpp
    gpu/compute_harness.cpp
    reference/sgp4_reference.cpp
    space/earth_frame_tests.cpp
    space/orbit_path_tests.cpp
    space/sgp4_init_tests.cpp
    space/sgp4_shader_tests.cpp
    space/sgp4_step_tests.cpp
    space/sgp4_tests.cpp
    test_main.cpp
)

target_include_directories(tests PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Absolute paths rather than relative ones: catch_discover_tests runs each case with its
# working directory at the binary's, so nothing relative to this list file would resolve.
# The shader path deliberately points at the tree the game ships from, so that the tests
# compile the very file that runs in the application rather than a copy of it.
target_compile_definitions(tests PRIVATE
    APSIS_TEST_DATA_DIR="${CMAKE_CURRENT_LIST_DIR}/data"
    APSIS_SHADER_DIR="${CMAKE_CURRENT_LIST_DIR}/../bin/data/core/shaders"
)

target_link_libraries(tests PRIVATE
    Catch2::Catch2
    game_lib
    sgp4_reference
)

# One CTest entry per Catch2 test case, rather than one for the whole binary. A failing
# PANDORA_ASSERT calls psnip_trap() and exits the process (see pandora/src/core/assert.hpp), which
# Catch2 cannot catch - so without this, one tripped assertion would take the entire run down with
# it and report nothing. Per-case registration confines the damage to the case that caused it.
catch_discover_tests(tests)
