project(forge)
set(CMAKE_CXX_STANDARD 20)

if(TARGET_PLATFORM_NATIVE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

find_package(pandora REQUIRED)
add_executable(forge ${SOURCE_FILES})

target_include_directories(
    forge PRIVATE 
    src/
    ${PANDORA_INCLUDE_DIRS}
)

target_link_directories(forge PRIVATE ${PANDORA_LIBRARY_DIRS})

target_link_libraries(forge PRIVATE pandora)

add_custom_command(TARGET forge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:forge> ${CMAKE_CURRENT_LIST_DIR}/bin
)

endif() # TARGET_PLATFORM_NATIVE
