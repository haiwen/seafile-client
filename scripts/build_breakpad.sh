#!/usr/bin/env bash

set -e

PWD=$(dirname "${BASH_SOURCE[0]}")

## get a copy of breakpad if not exist
if [ ! -d $PWD/../third_party/breakpad ]; then
  pushd $PWD/../third_party
  git clone https://github.com/Chilledheart/breakpad.git
  popd
fi

## get a copy of gyp if not exist
if [ ! -d $PWD/../third_party/gyp ]; then
  pushd $PWD/../third_party
  git clone https://chromium.googlesource.com/external/gyp.git
  popd
fi

## build breakpad
pushd $PWD/../third_party/breakpad

GYP_GENERATORS=ninja ../gyp/gyp --depth .
if [ -d out/Debug_Base ]; then
  ninja -C out/Debug_Base
elif [ -d out/Default ]; then
  ninja -C out/Default
else
  echo "unable to find build.ninja, exiting"
fi

echo "breakpad was built successfuly"
echo "now you can set up your PATH_TO_BREAKPAD_ROOT to $(pwd)"

popd


