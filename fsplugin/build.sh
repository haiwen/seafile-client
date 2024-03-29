#!/bin/bash

set -x
set -e

CURRENT_PWD="$(dirname "${BASH_SOURCE[0]}")"

if [ "$(uname -s)" != "Darwin" ]; then
  echo "don't run it if you are not using Mac OS X"
  exit -1
fi

export CC=$(xcrun -f clang)
export CXX=$(xcrun -f clang)
unset CFLAGS CXXFLAGS LDFLAGS

pushd $CURRENT_PWD
CONFIG=Debug
if [ a"$1" != "adebug" ]; then
  rm -rf CMakeCache.txt CMakeFiles
  CONFIG=Release
fi

if [ a"$1" != "auniversal" ]; then
    cmake -GXcode -DCMAKE_BUILD_TYPE="$CONFIG"
else
    cmake -GXcode -DCMAKE_BUILD_TYPE="$CONFIG" -DCMAKE_OSX_ARCHITECTURES='x86_64;arm64'
fi

xcodebuild clean
xcodebuild -jobs "$(sysctl -n hw.ncpu)" -configuration "$CONFIG"
popd

