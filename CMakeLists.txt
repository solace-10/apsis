cmake_minimum_required(VERSION 3.23) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pandora/cmake/shared)

#set(DAWN_FETCH_DEPENDENCIES ON)
#add_subdirectory("pandora/ext/dawn" EXCLUDE_FROM_ALL)

add_subdirectory("pandora")
add_subdirectory("game")