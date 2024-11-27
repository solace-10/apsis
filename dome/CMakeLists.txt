project(dome)
set(CMAKE_CXX_STANDARD 20)

if(TARGET_PLATFORM_NATIVE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

find_package(pandora REQUIRED)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(
    ${PROJECT_NAME} PRIVATE 
    src/
    ${PANDORA_INCLUDE_DIRS}
)

target_link_directories(${PROJECT_NAME} PRIVATE ${PANDORA_LIBRARY_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE pandora)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_LIST_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_LIST_DIR}/bin
)

endif() # TARGET_PLATFORM_NATIVE
