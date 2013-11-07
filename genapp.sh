#!/bin/sh

top_dir=${PWD}

dylibs="/usr/local/lib/libccnet.0.dylib /usr/local/lib/libseafile.0.dylib /usr/local/lib/libsearpc.1.dylib /usr/local/lib/libsearpc-json-glib.0.dylib /opt/local/lib/libcrypto.1.0.0.dylib /opt/local/lib/libuuid.16.dylib /opt/local/lib/libevent-2.0.5.dylib /opt/local/lib/libssl.1.0.0.dylib /opt/local/lib/libgio-2.0.0.dylib /opt/local/lib/libgmodule-2.0.0.dylib /opt/local/lib/libgobject-2.0.0.dylib /opt/local/lib/libgthread-2.0.0.dylib /opt/local/lib/libffi.6.dylib /opt/local/lib/libglib-2.0.0.dylib /opt/local/lib/libintl.8.dylib /opt/local/lib/libiconv.2.dylib /opt/local/lib/libsqlite3.0.dylib /opt/local/lib/libz.1.dylib /opt/local/lib/libjansson.4.dylib"


exes="`which ccnet` `which seaf-daemon`"
all=$dylibs" ${exes}"

target=seafile-applet
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
            mkdir -p libs
            cp -f `which ccnet` libs/
            cp -f `which seaf-daemon` libs/
            qmake -spec macx-xcode
            ;;

        "build" )
            echo "build ${target}.app for Mac OS X 10.6"
            rm -rf build
            xcodebuild -target ${target}
            rm -rf ${top_dir}/${target}.app
            cp -rf build/Release/${target}.app ${top_dir}/${target}.app
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
