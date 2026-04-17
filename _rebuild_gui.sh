#!/bin/bash
set -e
export PATH=/c/msys64/mingw64/bin:/usr/bin:/usr/local/bin
export TMPDIR=/f/Projects/simpler64/_tmp
export TMP=/f/Projects/simpler64/_tmp
export TEMP=/f/Projects/simpler64/_tmp
export SystemRoot="C:\\Windows"
export COMSPEC="C:\\Windows\\System32\\cmd.exe"

echo "--- rebuilding mupen64plus-core ---"
cd /f/Projects/simpler64/mupen64plus-core/build
/c/msys64/mingw64/bin/cmake.exe . 2>&1 | tail -5
/c/msys64/mingw64/bin/ninja.exe 2>&1 | tail -20

echo "--- rebuilding simple64-gui ---"
cd /f/Projects/simpler64/simple64-gui/build
/c/msys64/mingw64/bin/cmake.exe . 2>&1 | tail -5
/c/msys64/mingw64/bin/ninja.exe 2>&1

echo "--- copying to install dir ---"
cp /f/Projects/simpler64/simple64-gui/build/simple64-gui.exe /f/Projects/simpler64/simple64/simple64-gui.exe \
    && cp /f/Projects/simpler64/mupen64plus-core/build/libmupen64plus.dll /f/Projects/simpler64/simple64/libmupen64plus.dll \
    && echo "copied OK"
