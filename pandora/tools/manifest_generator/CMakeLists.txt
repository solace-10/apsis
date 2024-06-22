project(manifest_generator)
set(CMAKE_CXX_STANDARD 20)

include("../../cmake/shared/TargetPlatforms.cmake")
include("../../cmake/shared/FetchExternalDependencies.cmake")

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

add_executable(manifest_generator ${SOURCE_FILES})
target_link_libraries(manifest_generator PRIVATE 
    clipp
    nlohmann_json::nlohmann_json 
    xxHash::xxhash
)

add_custom_command(TARGET manifest_generator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:manifest_generator> ${CMAKE_CURRENT_LIST_DIR}/bin/$<TARGET_FILE_NAME:manifest_generator>
)