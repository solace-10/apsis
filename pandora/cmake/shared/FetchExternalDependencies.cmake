include(FetchContent)

message(STATUS "Pandora: fetching external dependencies...")

set(FETCHCONTENT_QUIET FALSE)

if(TARGET_PLATFORM_NATIVE)
    # By default Dawn uses Chromium's build system and dependency management.
    # Setting DAWN_FETCH_DEPENDENCIES clones the dependencies repositories.
    set(DAWN_FETCH_DEPENDENCIES ON)
    FetchContent_Declare(
        dawn
        GIT_REPOSITORY https://github.com/google/dawn.git
        GIT_TAG 2a26b19b72815804d0c3b91def9b82a2dc2178a5 # HEAD as of Jun 14, 2024
        GIT_PROGRESS TRUE
        GIT_SUBMODULES ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/dawn
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(dawn)

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

    option(DAWN_BUILD_SAMPLES "Enables building Dawn's samples" OFF)
    option(TINT_BUILD_CMD_TOOLS "Build the Tint command line tools" OFF)
    option(TINT_BUILD_DOCS "Build documentation" OFF)
    option(TINT_BUILD_TESTS "Build tests" OFF)

    if(TARGET_PLATFORM_WINDOWS)
        option(TINT_BUILD_MSL_WRITER "Build the MSL output writer" OFF)
        option(TINT_BUILD_SPV_READER "Build the SPIR-V input reader" OFF)
        option(TINT_BUILD_WGSL_READER "Build the WGSL input reader" ON)
        option(TINT_BUILD_GLSL_WRITER "Build the GLSL output writer" OFF)
        option(TINT_BUILD_GLSL_VALIDATOR "Build the GLSL output validator" OFF)
        option(TINT_BUILD_SPV_WRITER "Build the SPIR-V output writer" OFF)
        option(TINT_BUILD_WGSL_WRITER "Build the WGSL output writer" ON)
    endif()
endif()

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
    GIT_TAG 6f7b5d0ee2fe9948ab871a530888a6dc5c960700 # v1.90.8
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui)

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

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 013ac3beddff3dbffafd5177e7972067cd2b5083 # Head as of 27/06/2024
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(stb)