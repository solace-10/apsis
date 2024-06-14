include(FetchContent)

message(STATUS "Pandora: fetching external dependencies...")

set(FETCHCONTENT_QUIET FALSE)

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