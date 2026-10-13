@ECHO OFF
REM Installs the emsdk into pandora\ext
SET EXT_DIR=%~dp0..\ext
SET EMSDK_VERSION=4.0.17

IF NOT EXIST "%EXT_DIR%" MKDIR "%EXT_DIR%"
IF NOT EXIST "%EXT_DIR%\emsdk" git clone https://github.com/emscripten-core/emsdk.git "%EXT_DIR%\emsdk"

cd /d "%EXT_DIR%\emsdk"
cmd /c emsdk.bat install %EMSDK_VERSION%
cmd /c emsdk.bat activate --permanent %EMSDK_VERSION%
cmd /c emsdk_env.bat

cd /d "%~dp0"
