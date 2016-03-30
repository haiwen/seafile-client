#!/bin/bash
set -x
set -e
PWD=$(dirname "${BASH_SOURCE[0]}")

pushd $PWD/..

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    if [ -z "$USE_QT5" ]; then
        cmake -DBUILD_TESTING=on -DBUILD_SHIBBOLETH_SUPPORT=on -DUSE_QT5=OFF .
        make -j8 VERBOSE=1
        make test VERBOSE=1
    else
        set +e
        . /opt/qt56/bin/qt56-env.sh
        set -e
        cmake -DBUILD_TESTING=on -DBUILD_SHIBBOLETH_SUPPORT=on -DUSE_QT5=ON .
        make -j8 VERBOSE=1
        make test VERBOSE=1
    fi
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
    export PATH=/usr/local/opt/openssl/bin:/usr/local/opt/curl/bin:$PATH
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/opt/curl/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/openssl/lib/pkgconfig:/usr/local/opt/sqlite/lib/pkgconfig
    export CPPFLAGS="-I/usr/local/opt/openssl/include -I/usr/local/opt/sqlite/include -I/usr/local/opt/curl/include"
    export CXXFLAGS="$CXXFLAGS $CPPFLAGS -I/usr/local/include"
    export LDFLAGS="-L/usr/local/opt/openssl/lib -L/usr/local/opt/sqlite/lib -L/usr/local/opt/curl/lib -L/usr/local/lib"
    export PKG_CONFIG_LIBDIR=/usr/lib/pkgconfig:/usr/local/Library/ENV/pkgconfig/10.9
    export ACLOCAL_PATH=/usr/local/share/aclocal
    cmake -G Xcode -DBUILD_TESTING=on .
    xcodebuild -configuration Debug -target ALL_BUILD -jobs 8
    xcodebuild -configuration Debug -target RUN_TESTS
else
    printf "not supported platform"
    exit -1
fi

popd
