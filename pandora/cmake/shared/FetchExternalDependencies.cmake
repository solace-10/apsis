include(FetchContent)

message(STATUS "Pandora: fetching external dependencies...")

set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
  dawn
  GIT_REPOSITORY https://github.com/google/dawn.git
  GIT_TAG 57c158000cb079a03f19c625d2727cfb628c8e5e
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(dawn)