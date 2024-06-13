cmake_minimum_required(VERSION 3.23) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pandora/cmake/shared)

include("pandora/cmake/shared/FetchExternalDependencies.cmake")

#add_subdirectory("pandora")

add_executable(app "main.cpp")

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("pandora/ext/dawn" EXCLUDE_FROM_ALL)
target_link_libraries(app PRIVATE webgpu_cpp webgpu_dawn glfw webgpu_glfw)