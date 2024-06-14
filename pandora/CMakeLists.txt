cmake_minimum_required(VERSION 3.23)
project(pandora)
set(CMAKE_CXX_STANDARD 20)

include("cmake/shared/FetchExternalDependencies.cmake")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

add_library(pandora STATIC ${SOURCE_FILES})
include("cmake/shared/TargetPlatforms.cmake")
target_include_directories(pandora PUBLIC src/)
set_target_properties(pandora PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/pandora/lib)

if(TARGET_PLATFORM_WINDOWS)
    #add_library(pandora_static INTERFACE)
    #target_compile_options(pandora_static INTERFACE $<$<CONFIG:Debug>:/MTd> $<$<CONFIG:Release>:/MT>)

    target_compile_definitions(pandora PRIVATE UNICODE _UNICODE _HASEXCEPTIONS=0 NOMINMAX)
    target_compile_options(pandora PUBLIC $<$<CONFIG:Release>:/Ob2 /GL>)
    target_compile_options(pandora PUBLIC /Zi) # Debug information format: Program database.
    target_compile_options(pandora PUBLIC /MP) # Enable parallel compilation.
    #target_link_libraries(pandora PRIVATE pandora_static)
elseif(TARGET_PLATFORM_WEB)

endif()