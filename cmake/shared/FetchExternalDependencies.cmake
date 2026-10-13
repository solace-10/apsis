include(FetchContent)

# Every dependency below brings its own upstream CMakeLists, and between them they emit a dozen-odd
# warnings nobody here can act on: CMP0115 in bullet3, pre-3.10 cmake_minimum_required in clipp,
# freetype, glm and xxhash, syntax warnings in dawn. Consuming projects therefore configure with
# -Wno-dev/-Wno-deprecated (in their presets) and promote the warnings back for their own
# directories once this file has been included.
#
# It has to be done that way round rather than scoped here: cmake_diagnostic() only governs
# diagnostics issued while a directory is being processed, and several of these - CMP0115 among them
# - are issued at generate time, long after this file is done.

message(STATUS "Pandora: fetching external dependencies...")

set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

if(TARGET_PLATFORM_NATIVE)
    # By default Dawn uses Chromium's build system and dependency management.
    # Setting DAWN_FETCH_DEPENDENCIES clones the dependencies repositories.
    set(DAWN_FETCH_DEPENDENCIES ON)
    set(DAWN_BUILD_SAMPLES OFF)
    set(TINT_BUILD_CMD_TOOLS OFF)
    set(TINT_BUILD_DOCS OFF)
    set(TINT_BUILD_TESTS OFF)
    set(TINT_BUILD_MSL_WRITER OFF)
    set(TINT_BUILD_SPV_READER OFF)
    set(TINT_BUILD_GLSL_VALIDATOR OFF)
    set(TINT_BUILD_IR_BINARY OFF)
    FetchContent_Declare(
        dawn
        GIT_REPOSITORY https://github.com/google/dawn.git
        GIT_TAG 01940842b667a7812d0e4ca0ef4367fbec294241 # matches emdawnwebgpu port v20251002.162335 (emscripten 4.0.17)
        GIT_PROGRESS TRUE
        GIT_SUBMODULES ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/dawn
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(dawn)

    # Tint enables -pedantic-errors and -Weverything for clang. Newer clang versions
    # diagnose __COUNTER__ as a C2y extension under -Wc2y-extensions, which becomes
    # a hard error. Disable that specific warning on Tint targets only.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        get_property(_dawn_targets DIRECTORY "${dawn_SOURCE_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
        foreach(_tgt IN LISTS _dawn_targets)
            if(_tgt MATCHES "^tint_")
                target_compile_options(${_tgt} PRIVATE -Wno-c2y-extensions)
            endif()
        endforeach()
        # Tint targets are declared in subdirectories of dawn; iterate those too.
        function(_pandora_apply_tint_clang_workaround dir)
            get_property(_subtargets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
            foreach(_tgt IN LISTS _subtargets)
                if(_tgt MATCHES "^tint_")
                    target_compile_options(${_tgt} PRIVATE -Wno-c2y-extensions)
                endif()
            endforeach()
            get_property(_subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
            foreach(_sub IN LISTS _subdirs)
                _pandora_apply_tint_clang_workaround("${_sub}")
            endforeach()
        endfunction()
        _pandora_apply_tint_clang_workaround("${dawn_SOURCE_DIR}")
    endif()

    # clipp provides command line argument parsing functionality.
    # We use a fork of the original project as it has several fixes, including 
    # cmake and C++20 support.
    FetchContent_Declare(
        clipp
        GIT_REPOSITORY https://github.com/GerHobbelt/clipp.git
        GIT_TAG 02783b6782ebedbb2bebc2e6ceda738ee51c7df2 # v2023.10.12
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/clipp
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(clipp)

    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    # bullet3 lists several sources without a file extension, which CMP0115 diagnoses. Leaving the
    # policy unset gets the OLD behaviour anyway, so pinning it to OLD changes nothing except that
    # CMake stops warning about it. Unlike the rest of the noise here this one has to be silenced by
    # policy rather than by diagnostic state, because it is issued at generate time, where only the
    # root diagnostic state is available. Cleared again below so it does not reach anything else.
    set(CMAKE_POLICY_DEFAULT_CMP0115 OLD)
    FetchContent_Declare(
        bullet3
        GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
        GIT_TAG ebe1916b90acae8b13cd8c6b637d8327cdc64e94 # 3.17
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/bullet3
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(bullet3)
    unset(CMAKE_POLICY_DEFAULT_CMP0115)
endif()

FetchContent_Declare(
    debug-draw
    GIT_REPOSITORY https://github.com/glampert/debug-draw.git
    GIT_TAG dd78c2230adce80ab2e1e257e2e257cb25ea1312 # Head as of 24/07/2024
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/debug-draw
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(debug-draw)

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG 78213075654a688e9da6bc49f7f873d25c26d12c # v3.13.2
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/entt
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(entt)

FetchContent_Declare(
    delaunator-cpp
    URL https://raw.githubusercontent.com/delfrrr/delaunator-cpp/refs/heads/master/include/delaunator.hpp
    DOWNLOAD_NO_EXTRACT true
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/delaunator-cpp
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(delaunator-cpp)

FetchContent_Declare(
    FastNoiseLite
    GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
    GIT_TAG 7ccfbc16eb1c932568f177d63a9ba51d89bbe516 # v1.1.1
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/FastNoiseLite
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(FastNoiseLite)

# There is no harfbuzz in the emscripten sysroot, and freetype's FindHarfBuzz warns rather than
# staying quiet about it. The web build was never getting harfbuzz anyway, so say so outright and
# skip the search. Native builds find a real harfbuzz and keep using it.
if(TARGET_PLATFORM_WEB)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG 42608f77f20749dd6ddc9e0536788eaad70ea4b5 # VER-2-13-3
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/freetype
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(freetype)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e # v1.0.1
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/glm
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG dac07199cfd761113d966eb8ad739254e10df2fe # v1.92.7
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    imgui_markdown
    GIT_REPOSITORY https://github.com/solace-10/imgui_markdown.git
    GIT_TAG 85cd6062e041fa66d69426ed23b5495d329345ab # Head as of 11/04/2025
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui_markdown
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui_markdown)

FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG e55b9b54d5cf61f8e117cafb17846d7d742dd3b4 # v0.9.5
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/magic_enum
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
    xxhash
    GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
    GIT_TAG bbb27a5efb85b92a0486cf361a8635715a53f6ba # v0.8.2
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/xxhash
    SOURCE_SUBDIR cmake_unofficial
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(xxhash)

FetchContent_Declare(
    nlohmann-json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 # v3.11.3
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/nlohmann-json
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(nlohmann-json)

# libktx provides KTX2 reading and Basis Universal transcoding at runtime.
# It is pinned to the same version as the encoder vendored in tools/forge/bin/ktx,
# so that what Forge writes and what the engine reads never drift apart.
#
# Only the read half is linked, via the separate ktx_read target - libktx has no
# option to drop the writers from the main ktx target. That also keeps Ericsson's
# ETC codec out of the binary, which is under a non-OSS license and is only
# acceptable while it stays a build-time tool. It is reachable from ktx_read
# through KTX_FEATURE_ETC_UNPACK, which is software ETC decoding for KTX1 files
# on GPUs without ETC support and is not needed here, so that is turned off.
#
# KTX1 support is forced on despite nothing loading KTX1, because turning it off does
# not link: texture.c still calls ktxTexture1_constructFromStreamAndHeader.
#
# libktx refuses to configure as a shared library under Emscripten. Nothing here
# wants a shared one anyway, but an earlier dependency leaves BUILD_SHARED_LIBS on
# when Dawn is not in the mix to turn it off, so it is forced for this fetch and
# put back afterwards.
set(PANDORA_BUILD_SHARED_LIBS_BACKUP ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_KTX1 ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_ETC_UNPACK OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS OFF CACHE STRING "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_JNI OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_PY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG 936b655d10fe75f900967f524ba31005bebcbb47 # v4.4.2
    GIT_PROGRESS TRUE
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/ktx
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ktx)
set(BUILD_SHARED_LIBS ${PANDORA_BUILD_SHARED_LIBS_BACKUP} CACHE BOOL "" FORCE)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 013ac3beddff3dbffafd5177e7972067cd2b5083 # Head as of 27/06/2024
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
    tinygltf
    # Pinned to a commit rather than the 'release' branch: upstream deprecated the C++ v1/v2/v3
    # API in favour of a v3 C rewrite and moved tiny_gltf.h under attic/, so the branch URL 404s.
    # This is the last commit before that move, and is identical to the header we already used.
    URL https://raw.githubusercontent.com/syoyo/tinygltf/544290f60db8b505d96df509a008feae5d78eebe/tiny_gltf.h
    DOWNLOAD_NO_EXTRACT true
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinygltf
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(tinygltf)