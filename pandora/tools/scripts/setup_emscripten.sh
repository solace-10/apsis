#!/bin/bash
cd ../..
mkdir ext
cd ext
git clone https://github.com/emscripten-core/emsdk.git

cd emsdk
./emsdk install latest
./emsdk activate latest