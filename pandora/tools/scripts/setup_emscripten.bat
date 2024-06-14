
@ECHO OFF
cd ..\..\ext
IF NOT EXIST emsdk git clone https://github.com/emscripten-core/emsdk.git

cd emsdk
cmd /c emsdk.bat install latest
cmd /c emsdk.bat activate --permanent latest
cmd /c emsdk_env.bat

cd %~dp0