#!/bin/sh

top_dir=${PWD}


dylibs_com="/usr/local/lib/libccnet.0.dylib /usr/local/lib/libseafile.0.dylib /usr/local/lib/libsearpc.1.dylib /usr/local/lib/libsearpc-json-glib.0.dylib /opt/local/lib/libcrypto.1.0.0.dylib /opt/local/lib/libuuid.16.dylib /opt/local/lib/libevent-2.0.5.dylib /opt/local/lib/libssl.1.0.0.dylib /opt/local/lib/libgio-2.0.0.dylib /opt/local/lib/libgmodule-2.0.0.dylib /opt/local/lib/libgobject-2.0.0.dylib /opt/local/lib/libgthread-2.0.0.dylib /opt/local/lib/libffi.6.dylib /opt/local/lib/libglib-2.0.0.dylib /opt/local/lib/libintl.8.dylib /opt/local/lib/libiconv.2.dylib /opt/local/lib/libsqlite3.0.dylib /opt/local/lib/libz.1.dylib /opt/local/lib/libjansson.4.dylib"

dylibs_orig=$dylibs_com

all_orig=$dylibs_orig" /usr/local/bin/ccnet /usr/local/bin/seaf-daemon"

dylibs=$dylibs_com
all=$dylibs" /usr/local/bin/ccnet /usr/local/bin/seaf-daemon"

while [ $# -ge 1 ]; do
  case $1 in
    "dylib" )
      mkdir -p libs
      pushd libs
      for var in $all_orig ; do
          cp -f $var ./
          base=$(basename "$var")
          chmod 0744 $base
      done

      for var in $all ; do
          dyexe=$(basename "$var")
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
      done
      popd
      ;;

    "app" )
      echo "build seafile-client.app for Mac OS X 10.6"
      rm -rf build
      xcodebuild -target seafile-client
      rm -rf ${top_dir}/seafile-client.app
      cp -rf build/Release/seafile-client.app ${top_dir}/seafile-client.app
      echo "macdeployqt seafile-client.app"
      macdeployqt seafile-client.app
      ;;
    esac
    shift
done
