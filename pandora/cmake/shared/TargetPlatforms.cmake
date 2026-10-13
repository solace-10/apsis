if(TARGET_PLATFORM_NATIVE)
    target_compile_definitions(pandora PUBLIC TARGET_PLATFORM_NATIVE=1)
endif()

if(TARGET_PLATFORM_WINDOWS)
    target_compile_definitions(pandora PUBLIC TARGET_PLATFORM_WINDOWS=1)
endif()

if(TARGET_PLATFORM_WEB)
    target_compile_definitions(pandora PUBLIC TARGET_PLATFORM_WEB=1)
endif()