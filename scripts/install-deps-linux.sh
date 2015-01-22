#!/bin/bash
set -x
set -e

sudo add-apt-repository -y ppa:smspillaz/cmake-2.8.12
sudo add-apt-repository -y ppa:beineri/opt-qt54
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
sudo apt-get install -y gcc-4.8 g++-4.8
sudo apt-get install -y valac uuid-dev libevent-dev re2c libjansson-dev cmake cmake-data libqt4-dev
sudo apt-get install -y qt54base qt54translations qt54tools qt54webkit
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 50
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 50

git clone --depth=1 --branch=master git://github.com/haiwen/libsearpc.git deps/libsearpc
git clone --depth=1 --branch=master git://github.com/haiwen/ccnet.git deps/ccnet
git clone --depth=1 --branch=master git://github.com/haiwen/seafile.git deps/seafile
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
