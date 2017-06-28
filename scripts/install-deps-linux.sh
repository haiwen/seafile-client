#!/bin/bash
set -x
set -e
PWD=$(dirname "${BASH_SOURCE[0]}")

SEAFILE_BRANCH=master

# Temporary fix
sudo sed -i /rabbitmq\.com/d /etc/apt/sources.list
sudo sed -i /rabbitmq\.com/d /etc/apt/sources.list.d/*.list

sudo add-apt-repository -y ppa:smspillaz/cmake-2.8.12
sudo apt-get update -qq
sudo apt-get install -y valac uuid-dev libevent-dev re2c libjansson-dev cmake cmake-data

sudo add-apt-repository -y ppa:beineri/opt-qt562-trusty
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq

sudo apt-get install -y gcc-4.8 g++-4.8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 50
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 50

sudo apt-get install -y qt56base qt56translations qt56tools qt56webengine

# Fix errors like:
#
#   CMake Error at /opt/qt56/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake:9 (message):
#   Failed to find "GL/gl.h" in "/usr/include/libdrm"
#
# See https://github.com/Studio3T/robomongo/issues/1268
sudo apt-get install -y mesa-common-dev libglu1-mesa-dev

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
