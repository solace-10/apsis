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
    FetchContent_Declare(
        clipp
        GIT_REPOSITORY https://github.com/muellan/clipp.git
        GIT_TAG 2c32b2f1f7cc530b1ec1f62c92f698643bb368db # Head as of 20/06/2024, adding cmake support.
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/clipp
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(clipp)
endif()

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