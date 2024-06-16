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
endif()

FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG e55b9b54d5cf61f8e117cafb17846d7d742dd3b4 # v0.9.5
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/magic_enum
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(magic_enum)