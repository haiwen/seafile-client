#!/bin/sh

set -e

top_dir=${PWD}

dylibs="/usr/local/lib/libccnet.0.dylib /usr/local/lib/libseafile.0.dylib \
/usr/local/lib/libsearpc.1.dylib /opt/local/lib/libcrypto.1.0.0.dylib \
/opt/local/lib/libevent-2.0.5.dylib /opt/local/lib/libssl.1.0.0.dylib \
/opt/local/lib/libgio-2.0.0.dylib /opt/local/lib/libgmodule-2.0.0.dylib \
/opt/local/lib/libgobject-2.0.0.dylib /opt/local/lib/libgthread-2.0.0.dylib \
/opt/local/lib/libffi.6.dylib /opt/local/lib/libglib-2.0.0.dylib \
/opt/local/lib/libintl.8.dylib /opt/local/lib/libiconv.2.dylib \
/opt/local/lib/libsqlite3.0.dylib /opt/local/lib/libz.1.dylib \
/opt/local/lib/libjansson.4.dylib /opt/local/lib/libcurl.4.dylib \
/opt/local/lib/libidn.11.dylib"

exes="`which ccnet` `which seaf-daemon`"
all=$dylibs" ${exes}"

target=seafile-applet
configuration=Release
num_cpus=$(sysctl -n hw.ncpu)
function change_otool() {
    DIR=$1
    pushd ${DIR}
    for var in $all ; do
        dyexe=$(basename "$var")
        if [ -f $dyexe ] ; then
            echo "Deal with "$dyexe
            for libpath in $dylibs ; do
                lib=$(basename $libpath)
                if [ "$lib" = "$dyexe" ] ; then
                    echo "install_name_tool -id @loader_path/../Frameworks/$lib $dyexe"
                    install_name_tool -id @loader_path/../Frameworks/$lib $dyexe
                else
                    echo "install_name_tool -change $libpath @loader_path/../Frameworks/$lib $dyexe"
                    install_name_tool -change $libpath @loader_path/../Frameworks/$lib $dyexe
                fi
            done
        fi
    done
    popd
}

while [ $# -ge 1 ]; do
    case $1 in
        "xcode" )
            export CC=clang
            export CXX=clang++
            cmake -G Xcode -DCMAKE_BUILD_TYPE=${configuration} .
            ;;

        "build" )
            echo "build ${target}.app with -j${num_cpus}"
            rm -rf Release
            xcodebuild -target ${target} -configuration $configuration -jobs $num_cpus
            rm -rf ${top_dir}/${target}.app
            cp -rf Release/${target}.app ${top_dir}/${target}.app
            ;;

        # use xctool, provide better output (experimental)
        "build2" )
            echo "build ${target}.app with xctool -j${num_cpus}"
            rm -rf Release
            xctool -find-target ${target} -configuration $configuration -jobs $num_cpus
            rm -rf ${top_dir}/${target}.app
            cp -rf Release/${target}.app ${top_dir}/${target}.app
            ;;

        "libs" )
            mkdir -p ${top_dir}/${target}.app/Contents/Frameworks
            pushd ${top_dir}/${target}.app/Contents/Frameworks
            for var in $dylibs ; do
                cp -f $var ./
                base=$(basename "$var")
                chmod 0744 $base
            done
            popd
            ;;

        "otool" )
            echo "macdeployqt ${target}.app"
            macdeployqt ${target}.app
            change_otool ${top_dir}/${target}.app/Contents/Resources
            change_otool ${top_dir}/${target}.app/Contents/Frameworks
            ;;

        "dmg" )
            echo "macdeployqt ${target}.app -dmg"
            macdeployqt ${target}.app -dmg
            ;;

    esac
    shift
done
