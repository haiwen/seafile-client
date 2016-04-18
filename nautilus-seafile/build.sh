#!/bin/bash

NINJA=$(which ninja)
GENERATOR=Ninja
WORKER=ninja
if [ -z $NINJA ]; then
  GENERATOR="Unix Makefiles"
  WORKER=make
fi

set -x
set -e

CURRENT_PWD="$(dirname "${BASH_SOURCE[0]}")"

if [ "$(uname -s)" != "Linux" ]; then
  echo "don't run it if you are not using Linux"
  exit -1
fi

pushd $CURRENT_PWD
CONFIG=Debug
if [ a"$1" != "adebug" ]; then
  rm -rf CMakeCache.txt CMakeFiles
  CONFIG=Release
fi
cmake -G "$GENERATOR" \
  -DCMAKE_BUILD_TYPE="$CONFIG" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=on
$WORKER
popd

