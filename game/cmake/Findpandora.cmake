set(PANDORA_INCLUDE_DIRS "${pandora_SOURCE_DIR}/src/")
set(PANDORA_LIBRARY_DIRS "${pandora_SOURCE_DIR}/libs/pandora/${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_HOST_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}")

#if(WIN32)
#    set(PANDORA_CRASH_HANDLER "${pandora_SOURCE_DIR}/../build/$<IF:$<CONFIG:Debug>,Debug,Release>/crashpad_handler.exe")
#else()
#    set(PANDORA_CRASH_HANDLER "${pandora_SOURCE_DIR}/../build/crashpad_handler")
#endif()