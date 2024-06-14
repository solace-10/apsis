cmake_minimum_required(VERSION 3.23) 
project(game)
set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/src FILES ${SOURCE_FILES})

find_package(pandora REQUIRED)
add_executable(game ${SOURCE_FILES})

target_include_directories(game PRIVATE ${PANDORA_INCLUDE_DIRS})
target_link_directories(game PRIVATE ${PANDORA_LIBRARY_DIRS})
target_link_libraries(game PRIVATE pandora)

if(TARGET_PLATFORM_NATIVE)
    target_link_libraries(game PRIVATE webgpu_cpp webgpu_dawn glfw webgpu_glfw)
elseif(TARGET_PLATFORM_WEB)
    set_target_properties(game PROPERTIES SUFFIX ".html")
    target_link_options(game PRIVATE "-sUSE_WEBGPU=1" "-sUSE_GLFW=3")
endif()