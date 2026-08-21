project(game)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

# Everything except the entry point goes into a library, so that the test suite can reach it
# without a second, hand-maintained list of which sources are testable. main.cpp is the only
# thing the executable adds - see game/tests/CMakeLists.txt.
list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp")

find_package(pandora REQUIRED)

add_library(game_lib STATIC ${SOURCE_FILES})

# PUBLIC rather than PRIVATE throughout: anything linking game_lib - the game itself, the tests -
# needs the same include paths and the same engine behind it.
target_include_directories(
    game_lib PUBLIC
    src/
    ${PANDORA_INCLUDE_DIRS}
)

target_link_directories(game_lib PUBLIC ${PANDORA_LIBRARY_DIRS})
target_link_libraries(game_lib PUBLIC pandora)

add_executable(game src/main.cpp)

# WHOLE_ARCHIVE restores exactly what the executable had before game_lib existed: every object,
# whether or not anything references a symbol in it.
#
# This is not an optimisation detail, it is a correctness one. A static library only yields the
# objects needed to resolve undefined symbols, so a translation unit whose entire purpose is a
# static initialiser gets dropped - and it fails silently, because nothing was ever named.
# src/emscripten/bindings.cpp is precisely that: nothing outside it references its symbols, so
# without this the EMSCRIPTEN_BINDINGS block vanishes and the webapp quietly loses
# setGroupFilterEnabled (module.ts guards on its presence, so there is not even an error).
#
# The tests link game_lib normally, so they stay lean and pull in only what they touch.
target_link_libraries(game PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,game_lib>")

if(TARGET_PLATFORM_NATIVE)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:game> ${CMAKE_CURRENT_LIST_DIR}/bin
    )
elseif(TARGET_PLATFORM_WEB)
    target_link_options(game PRIVATE
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/src/emscripten/shell.html"
        "--bind" # Support for embind so we can interact with the frontend.
    )

    # Generate asset manifest using forge tool
if(WIN32)
    set(FORGE_EXECUTABLE "${CMAKE_SOURCE_DIR}/pandora/tools/forge/bin/forge.exe")
else()
    set(FORGE_EXECUTABLE "${CMAKE_SOURCE_DIR}/pandora/tools/forge/bin/forge")
endif()
    set(MANIFEST_OUTPUT "${CMAKE_CURRENT_LIST_DIR}/bin/manifest.json")

    add_custom_command(
        OUTPUT "${MANIFEST_OUTPUT}"
        COMMAND "${FORGE_EXECUTABLE}" manifest
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating asset manifest..."
        DEPENDS "${FORGE_EXECUTABLE}"
    )

    add_custom_target(generate_manifest DEPENDS "${MANIFEST_OUTPUT}")
    add_dependencies(game generate_manifest)

    # Assets have to be compiled before they can be hashed into the manifest.
    add_dependencies(generate_manifest forge_compile_assets)

    set_target_properties(game PROPERTIES
        SUFFIX ".html"
        LINK_DEPENDS "${MANIFEST_OUTPUT}"
    )
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:game>/$<TARGET_FILE_BASE_NAME:game>.js ${CMAKE_CURRENT_LIST_DIR}/../webapp/static
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:game>/$<TARGET_FILE_BASE_NAME:game>.wasm ${CMAKE_CURRENT_LIST_DIR}/../webapp/static
    )
endif()
