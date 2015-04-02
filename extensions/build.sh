#!/bin/bash

set -e

unset -v CPPFLAGS CFLAGS CXXFLAGS LDFLAGS

SCRIPT=$(readlink -f "$0")
SRCDIR=$(dirname "${SCRIPT}")

cd $SRCDIR

rm -rf CMakeCache.txt CMakeFiles
export CXX=g++
cmake -DX32=ON -G "MSYS Makefiles" .
make

rm -rf CMakeCache.txt CMakeFiles
export CXX=x86_64-w64-mingw32-g++
cmake -G "MSYS Makefiles" .
make
