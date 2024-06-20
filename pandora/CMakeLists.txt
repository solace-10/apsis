project(pandora)
set(CMAKE_CXX_STANDARD 20)

include("cmake/shared/FetchExternalDependencies.cmake")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

add_library(pandora STATIC ${SOURCE_FILES})
include("cmake/shared/TargetPlatforms.cmake")
target_include_directories(pandora PUBLIC src/)
set_target_properties(pandora PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/pandora/lib)

target_link_libraries(pandora PUBLIC magic_enum::magic_enum nlohmann_json::nlohmann_json)

if(TARGET_PLATFORM_WINDOWS)
    target_compile_definitions(pandora PUBLIC UNICODE _UNICODE _HASEXCEPTIONS=0 NOMINMAX)
    target_compile_options(pandora PUBLIC $<$<CONFIG:Release>:/Ob2 /GL>)
    target_compile_options(pandora PUBLIC /Zi) # Debug information format: Program database.
    target_compile_options(pandora PUBLIC /MP) # Enable parallel compilation.
    target_link_libraries(pandora PUBLIC webgpu_cpp webgpu_dawn glfw webgpu_glfw)
elseif(TARGET_PLATFORM_LINUX)
    target_link_libraries(pandora PUBLIC webgpu_cpp webgpu_dawn glfw webgpu_glfw)
elseif(TARGET_PLATFORM_WEB)
    target_compile_options(pandora PUBLIC 
        "-O0"
    )
    target_compile_definitions(pandora PRIVATE 
        "VFS_WEB_HOST=\"http://127.0.0.1:8000\""
    )
    target_link_options(pandora PUBLIC 
        "-sUSE_WEBGPU=1" 
        "-sUSE_GLFW=3" 
        "-sFETCH" 
        "-sSAFE_HEAP" 
        "-sALLOW_MEMORY_GROWTH" 
        "-sASSERTIONS=2" 
        "-sSTACK_SIZE=5MB"
        #"-fsanitize=address"
    )
endif()

if(TARGET_PLATFORM_NATIVE)
    add_subdirectory("tools/manifest_generator")
endif()