@echo off
setlocal

if "%1"=="" (
    echo Usage: build_web.bat [debug^|release]
    exit /b 1
)

if /i "%1"=="debug" (
    cmake --build ../../build/debug-web --target setup_compile_commands game
) else if /i "%1"=="release" (
    cmake --build ../../build/release-web --target setup_compile_commands game
) else (
    echo Invalid argument: %1
    echo Usage: build_web.bat [debug^|release]
    exit /b 1
)
