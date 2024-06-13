cmake_minimum_required(VERSION 3.23) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pandora/cmake/shared)

include("pandora/cmake/shared/FetchExternalDependencies.cmake")

#add_subdirectory("pandora")

find_package(dawn CONFIG REQUIRED)

add_executable(app "main.cpp")