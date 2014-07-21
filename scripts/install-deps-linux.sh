#!/bin/bash
set -x
set -e

sudo add-apt-repository -y ppa:smspillaz/cmake-2.8.12
sudo add-apt-repository -y ppa:beineri/opt-qt54
sudo apt-get update -qq
sudo apt-get install -y valac uuid-dev libevent-dev re2c libjansson-dev cmake cmake-data libqt4-dev
sudo apt-get install -y qt54base qt54translations qt54tools qt54webkit

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
