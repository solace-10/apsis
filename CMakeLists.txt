cmake_minimum_required(VERSION 3.22) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standardq

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pandora/cmake/shared)

# Enable compile commands for clangd, which is used by Cursor.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(setup_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
    DEPENDS "${CMAKE_BINARY_DIR}/compile_commands.json"
    COMMENT "Copying compile_commands.json for clangd integration..."
)

add_subdirectory("pandora")
add_subdirectory("dome")
add_subdirectory("forge")
add_subdirectory("game")