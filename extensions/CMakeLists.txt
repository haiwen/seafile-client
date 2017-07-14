CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(seafile_shell_ext)

OPTION(X32 "Build 32 bit instead of 64bit dll" OFF)

IF (X32)
  SET(WINVER 0x0501)
  SET(DLL_NAME seafile_shell_ext)
  set(CMAKE_RC_COMPILER windres)
ELSE(X32)
  SET(WINVER 0x0502)
  SET(DLL_NAME seafile_shell_ext64)
  set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
ENDIF()

IF (NOT (${CMAKE_BUILD_TYPE} MATCHES Release))
  SET(CMAKE_BUILD_TYPE Debug)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
ENDIF()
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -std=c++0x")

ADD_DEFINITIONS(-D_WIN32_WINNT=${WINVER} -DWINVER=${WINVER})

SET(CMAKE_CXX_CREATE_SHARED_LIBRARY
    "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> ${CMAKE_GNULD_IMAGE_VERSION} <OBJECTS> <LINK_LIBRARIES>")

SET(ext_sources
  dll.cpp
  shell-ext.cpp
  class-factory.cpp
  context-menu.cpp
  icon-overlay.cpp
  log.cpp
  applet-connection.cpp
  commands.cpp
  ext-utils.cpp
  i18n.cpp
  seafile_shell_ext.def
  seafile_shell_ext.rc
)

SET_SOURCE_FILES_PROPERTIES(context-menu.cpp PROPERTIES COMPILE_FLAGS -fpermissive)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
LINK_LIBRARIES(uuid oleaut32 ole32 ws2_32 shlwapi userenv)

ADD_LIBRARY(seafile_shell_ext SHARED ${ext_sources})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

SET_TARGET_PROPERTIES(seafile_shell_ext PROPERTIES
  LINK_FLAGS "-static -lstdc++ -Wl,--enable-stdcall-fixup"
  OUTPUT_NAME "${DLL_NAME}"
  PREFIX "")
