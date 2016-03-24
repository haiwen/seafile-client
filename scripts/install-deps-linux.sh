#!/bin/bash
set -x
set -e
PWD=$(dirname "${BASH_SOURCE[0]}")

## default (branch build, PR build)
## for tag build, we use the tag version for seafile
SEAFILE_BRANCH="$TRAVIS_BRANCH"

function set_seafile_branch_from_cmake() {
    SEAFILE_VERSION_MAJOR=$(grep "SEAFILE_CLIENT_VERSION_MAJOR" $PWD/../CMakeLists.txt |head -n1 | cut -f 2-2 -d' ' | cut -f 1-1 -d ')')
    SEAFILE_VERSION_MINOR=$(grep "SEAFILE_CLIENT_VERSION_MINOR" $PWD/../CMakeLists.txt |head -n1 | cut -f 2-2 -d' ' | cut -f 1-1 -d ')')
    SEAFILE_BRANCH="${SEAFILE_VERSION_MAJOR}.${SEAFILE_VERSION_MINOR}"
}

## branch build and not a tag build
if [ "a$TRAVIS_PULL_REQUEST" = "afalse" -a -z "$TRAVIS_TAG" -a "$TRAVIS_BRANCH" != "master" ]; then
    set_seafile_branch_from_cmake
fi

## not a ci build
if [ -z "$SEAFILE_BRANCH" ]; then
    SEAFILE_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    if [ -z "$SEAFILE_BRANCH" -o "$SEAFILE_BRANCH" = "HEAD" ]; then
        set_seafile_branch_from_cmake
    fi
fi

## update this when major version bump
if [ "$SEAFILE_BRANCH" = "4.4" ]; then
    SEAFILE_BRANCH=master
fi

# Temporary fix
sudo sed -i /rabbitmq\.com/d /etc/apt/sources.list
sudo sed -i /rabbitmq\.com/d /etc/apt/sources.list.d/*.list

sudo add-apt-repository -y ppa:smspillaz/cmake-2.8.12
sudo apt-get update -qq
sudo apt-get install -y valac uuid-dev libevent-dev re2c libjansson-dev cmake cmake-data libqt4-dev

if [ ! -z "$USE_QT5" ]; then
    sudo add-apt-repository -y ppa:beineri/opt-qt56-trusty
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
    sudo apt-get update -qq

    sudo apt-get install -y gcc-4.8 g++-4.8
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 50
    sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 50

    sudo apt-get install -y qt56base qt56translations qt56tools qt56webengine
fi

git clone --depth=1 --branch=master git://github.com/haiwen/libsearpc.git deps/libsearpc
git clone --depth=1 --branch=master git://github.com/haiwen/ccnet.git deps/ccnet
git clone --depth=1 --branch="$SEAFILE_BRANCH" git://github.com/haiwen/seafile.git deps/seafile
pushd deps/libsearpc
./autogen.sh && ./configure
make -j8 && sudo make install
popd

pushd deps/ccnet
./autogen.sh && ./configure --enable-client --disable-server
make -j8 && sudo make install
popd

pushd deps/seafile
./autogen.sh && ./configure --disable-fuse --disable-server --enable-client
make -j8 && sudo make install
popd
