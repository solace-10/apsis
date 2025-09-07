project(pandora)
set(CMAKE_CXX_STANDARD 20)

include("cmake/shared/FetchExternalDependencies.cmake")

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

if(TARGET_PLATFORM_NATIVE)
    set(IMGUI_NODE_EDITOR_SOURCE_DIR ext/imgui-node-editor)
    set(SOURCE_FILES_IMGUI_NODE_EDITOR
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/crude_json.cpp
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/crude_json.h
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_bezier_math.h
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_bezier_math.inl
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_canvas.cpp
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_canvas.h
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_extra_math.h
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_extra_math.inl
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor_api.cpp
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor_internal.h
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor_internal.inl
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor.cpp
        ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor.h
    )
endif()

add_library(pandora STATIC ${SOURCE_FILES} ${SOURCE_FILES_IMGUI} ${SOURCE_FILES_IMGUI_NODE_EDITOR})
include("cmake/shared/TargetPlatforms.cmake")

target_include_directories(pandora PUBLIC 
    src/
    ${IMGUI_SOURCE_DIR}
    ${IMGUI_SOURCE_DIR}/backends/
    ${IMGUI_NODE_EDITOR_SOURCE_DIR}
    ext/imgui_markdown/
    ext/delaunator-cpp/
    ext/entt/single_include/
    ext/stb/
    ext/tinygltf/
)

target_include_directories(pandora PRIVATE
    ext/debug-draw/
    ext/freetype/include/
)

set_target_properties(pandora PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/pandora/lib)

target_compile_definitions(pandora PUBLIC
    DEBUG_DRAW_CXX11_SUPPORTED
    DEBUG_DRAW_VEC3_TYPE_DEFINED
    DEBUG_DRAW_VERTEX_BUFFER_SIZE=32768
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_LEFT_HANDED
    IMGUI_ENABLE_FREETYPE
    IMGUI_USER_CONFIG="imgui/imgui_config.hpp"
    IMGUI_DEFINE_MATH_OPERATORS
)

target_link_libraries(pandora PUBLIC 
    freetype
    glm::glm
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

if (TARGET_PLATFORM_NATIVE)
    target_include_directories(pandora PUBLIC ext/bullet3/src)
    target_link_libraries(pandora PUBLIC webgpu_cpp webgpu_dawn glfw webgpu_glfw)
    target_link_libraries(pandora PUBLIC Bullet3Common BulletDynamics BulletCollision LinearMath)
elseif(TARGET_PLATFORM_WEB)
    target_compile_options(pandora PUBLIC 
        "-O0"
    )
    target_compile_definitions(pandora PRIVATE 
        "VFS_WEB_HOST=\"http://127.0.0.1:8000\""
    )
    target_link_options(pandora PUBLIC 
        "-sDEFAULT_TO_CXX"
        "-sUSE_BULLET=1"
        "-sUSE_WEBGPU=1" 
        "-sUSE_GLFW=3" 
        "-sFETCH" 
        "-sSAFE_HEAP" 
        "-sALLOW_MEMORY_GROWTH" 
        "-sASSERTIONS=2" 
        "-sSTACK_SIZE=5MB"
        "-sDISABLE_EXCEPTION_CATCHING"
        #"-fsanitize=address"
    )
endif()