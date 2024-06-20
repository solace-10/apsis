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
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:game> ${CMAKE_CURRENT_LIST_DIR}/bin
    )
elseif(TARGET_PLATFORM_WEB)
    set_target_properties(game PROPERTIES SUFFIX ".html")
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:game>/$<TARGET_FILE_BASE_NAME:game>.html ${CMAKE_CURRENT_LIST_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:game>/$<TARGET_FILE_BASE_NAME:game>.js ${CMAKE_CURRENT_LIST_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:game>/$<TARGET_FILE_BASE_NAME:game>.wasm ${CMAKE_CURRENT_LIST_DIR}/bin
    )
endif()
