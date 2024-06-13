include(FetchContent)

message(STATUS "Pandora: fetching external dependencies...")

set(FETCHCONTENT_QUIET FALSE)

# By default Dawn uses Chromium's build system and dependency management.
# Setting DAWN_FETCH_DEPENDENCIES clones the dependencies repositories.
#set(DAWN_FETCH_DEPENDENCIES ON)
#FetchContent_Declare(
#  dawn
#  GIT_REPOSITORY https://github.com/google/dawn.git
#  GIT_TAG 1762328886136518b56961f8388d90472dc2c973
#  GIT_PROGRESS TRUE
#  EXCLUDE_FROM_ALL
#)
#FetchContent_MakeAvailable(dawn)