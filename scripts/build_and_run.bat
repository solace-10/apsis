cmake --build ../../build/debug-windows --target setup_compile_commands game
if %errorlevel% equ 0 (
    echo Build successful, launching game...
    cd ..\..\game\bin
    game.exe
    cd ..\..\pandora\scripts
) else (
    echo Build failed with error code %errorlevel%
)
