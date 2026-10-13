project(pandora)
set(CMAKE_CXX_STANDARD 20)

include("cmake/shared/Diagnostics.cmake")
include("cmake/shared/FetchExternalDependencies.cmake")

# Pandora is a consumer of its own dependencies, so it restores its warnings the same way the
# top-level project does - see Diagnostics.cmake for what is being undone and why it has to be here
# rather than before the include above.
pandora_enable_project_diagnostics()

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

set(IMGUI_SOURCE_DIR ext/imgui)
set(SOURCE_FILES_IMGUI
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
    ${IMGUI_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${IMGUI_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
    ${IMGUI_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
    ${IMGUI_SOURCE_DIR}/misc/freetype/imgui_freetype.h
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui.h
    ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
)

add_library(pandora STATIC ${SOURCE_FILES} ${SOURCE_FILES_IMGUI})
include("cmake/shared/TargetPlatforms.cmake")

target_include_directories(pandora PUBLIC
    src/
)

# Third-party includes marked as SYSTEM to suppress warnings
target_include_directories(pandora SYSTEM PUBLIC
    ${IMGUI_SOURCE_DIR}
    ${IMGUI_SOURCE_DIR}/backends/
    ext/imgui_markdown/
    ext/delaunator-cpp/
    ext/entt/single_include/
    ext/stb/
    ext/tinygltf/
)

target_include_directories(pandora SYSTEM PRIVATE
    ext/debug-draw/
    ext/FastNoiseLite/Cpp/
    ext/freetype/include/
)

set_target_properties(pandora PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/pandora/lib)

target_compile_definitions(pandora PUBLIC
    DEBUG_DRAW_CXX11_SUPPORTED
    DEBUG_DRAW_VEC3_TYPE_DEFINED
    DEBUG_DRAW_VERTEX_BUFFER_SIZE=32768
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    IMGUI_ENABLE_FREETYPE
    IMGUI_USER_CONFIG="imgui/imgui_config.hpp"
    IMGUI_DEFINE_MATH_OPERATORS
    IMGUI_IMPL_WEBGPU_BACKEND_DAWN
)

target_link_libraries(pandora PUBLIC 
    freetype
    glm::glm
    ktx_read
    magic_enum::magic_enum 
    nlohmann_json::nlohmann_json
    xxHash::xxhash
)

if(TARGET_PLATFORM_WINDOWS)
    target_compile_definitions(pandora PUBLIC UNICODE _UNICODE _HASEXCEPTIONS=0 NOMINMAX)
    target_compile_options(pandora PUBLIC $<$<CONFIG:Release>:/Ob2 /GL>)
    target_compile_options(pandora PUBLIC /Zi) # Debug information format: Program database.
    target_compile_options(pandora PUBLIC /MP) # Enable parallel compilation.
endif()

# Compile the assets described in game/assets into game/bin/data/core.
# Every platform needs this, as the compiled assets are what the game loads.
# Forge caches its output, so this is cheap when nothing has changed.
if(CMAKE_HOST_WIN32)
    set(FORGE_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/tools/forge/bin/forge-windows-amd64.exe")
elseif(CMAKE_HOST_APPLE)
    set(FORGE_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/tools/forge/bin/forge-darwin-arm64")
else()
    set(FORGE_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/tools/forge/bin/forge-linux-amd64")
endif()

add_custom_target(forge_compile_assets
    COMMAND ${FORGE_EXECUTABLE} assets
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Compiling assets..."
)

add_dependencies(pandora forge_compile_assets)

if (TARGET_PLATFORM_NATIVE)
    target_include_directories(pandora SYSTEM PUBLIC ext/bullet3/src)
    target_link_libraries(pandora PUBLIC webgpu_cpp webgpu_dawn glfw webgpu_glfw)
    target_link_libraries(pandora PUBLIC Bullet3Common BulletDynamics BulletCollision LinearMath)
elseif(TARGET_PLATFORM_WEB)
    # Generate manifest.json before building for web.
    # This is called every time, with Forge regenerating the manifest as necessary.

    add_custom_target(forge_generate_manifest
        COMMAND ${FORGE_EXECUTABLE} manifest
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "Generating manifest.json for web build..."
    )

    add_custom_target(forge_upload
        COMMAND ${FORGE_EXECUTABLE} upload
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "Uploading files for web build..."
    )
    
    add_dependencies(pandora forge_generate_manifest forge_upload)
    add_dependencies(forge_upload forge_generate_manifest)

    # The manifest must describe compiled assets, never their sources.
    add_dependencies(forge_generate_manifest forge_compile_assets)

    target_compile_options(pandora PUBLIC
        "--use-port=bullet"
        "--use-port=contrib.glfw3"
        "--use-port=emdawnwebgpu"
    )


    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(pandora PUBLIC
            "-O0"
        )
        
        #target_link_options(pandora PUBLIC
        #    "-sASSERTIONS=2"
        #    "-fsanitize=address"
        #)
    else()
        target_compile_options(pandora PUBLIC
            "-O3"
            "-flto"
        )
        target_link_options(pandora PUBLIC
            "-O3"
            "-flto"
            "-sASSERTIONS=0"
            "-sEVAL_CTORS=2"
        )
    endif()
    
    target_compile_definitions(pandora PRIVATE
        "VFS_WEB_HOST=\"https://forge-cloudflare-worker.wings-of-steel.workers.dev/\""        
    )
    target_compile_options(pandora PUBLIC
        "-msimd128"
    )
    target_link_options(pandora PUBLIC
        "-sDEFAULT_TO_CXX"
        "-sENVIRONMENT=web" # Removes Node.js support
        "--use-port=bullet"
        "--use-port=contrib.glfw3"
        "--use-port=emdawnwebgpu"
        "-sFETCH"
        "-sALLOW_MEMORY_GROWTH"
        "-sNO_EXIT_RUNTIME=1"
        "-sSTACK_SIZE=5MB"
        "-sDISABLE_EXCEPTION_CATCHING"
        "--embed-file=${CMAKE_CURRENT_LIST_DIR}/../game/bin/manifest.json@/"
    )
endif()
