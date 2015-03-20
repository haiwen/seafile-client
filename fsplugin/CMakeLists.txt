CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

PROJECT(seafile-fsplugin)
SET(PROJECT_VERSION "1.0.0")

IF (NOT (${CMAKE_BUILD_TYPE} MATCHES Release))
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

SET(CODESIGN_CERTIFICATE "-" CACHE STRING "the certificate to sign code")

MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

IF (NOT APPLE)
    MESSAGE(FATAL_ERROR "Support only on OS X")
ENDIF()

IF (NOT CMAKE_GENERATOR STREQUAL Xcode)
    MESSAGE(FATAL_ERROR "Support only for Xcode generator")
ENDIF()

## Build with warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wsign-compare -Wno-long-long -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wsign-compare -Wno-long-long -Wno-unused-parameter -Woverloaded-virtual")

## Find OSX SDK 10.10

SET(XCRUN_COMMAND "xcrun" "--sdk" "macosx10.10" "--show-sdk-path")
SET(SDK_PATH "" CACHE STRING "path of osx 10.10 sdk")
EXECUTE_PROCESS(
  COMMAND ${XCRUN_COMMAND}
  RESULT_VARIABLE HAD_ERROR
  OUTPUT_VARIABLE SDK_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
IF(HAS_ERROR)
  MESSAGE(WARNING "unable to run xcrun, using default Xcode path")
ENDIF()

## Find the dependencies

FIND_LIBRARY(AppKit_LIBRARY AppKit)
FIND_LIBRARY(FinderSync_LIBRARY FinderSync)
FIND_LIBRARY(PlugInKit_LIBRARY PlugInKit
  PATHS ${SDK_PATH}/System/Library/PrivateFrameworks/
        /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/System/Library/PrivateFrameworks/)

if (NOT PlugInKit_LIBRARY)
  MESSAGE(FATAL_ERROR "PlugInKit not found")
endif()

ADD_DEFINITIONS(-fapplication-extension -fmodules)
SET(CMAKE_EXE_LINKER_FLAGS "-fapplication-extension -e _NSExtensionMain -fobjc-arc -fobjc-link-runtime ${CMAKE_EXE_LINKER_FLAGS}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_EXECUTABLE(seafile-finder-sync MACOSX_BUNDLE
  FinderSync.mm
  FinderSyncClient.mm
)

SET_TARGET_PROPERTIES(seafile-finder-sync PROPERTIES
  OUTPUT_NAME "Seafile FinderSync"
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.10"
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
  XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11"
  XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
  )

TARGET_LINK_LIBRARIES(seafile-finder-sync
  ${FinderSync_LIBRARY}
  ${PlugInKit_LIBRARY}
  ${AppKit_LIBRARY}
)

FIND_PROGRAM(CODESIGN codesign)

SET(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/\${CONFIGURATION}/Seafile FinderSync.app")
SET(TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/Seafile FinderSync.appex")
ADD_CUSTOM_COMMAND(TARGET seafile-finder-sync
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E remove -f ${TARGET_DIR} &&
  ${CMAKE_COMMAND} -E copy_directory ${BUILD_DIR} ${TARGET_DIR} &&
  ${CMAKE_COMMAND} -E make_directory ${TARGET_DIR}/Contents/Resources &&
  ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/../seafile.icns ${TARGET_DIR}/Contents/Resources/ &&
  ${CMAKE_COMMAND} -E copy status-done.icns ${TARGET_DIR}/Contents/Resources/ &&
  ${CMAKE_COMMAND} -E copy status-error.icns ${TARGET_DIR}/Contents/Resources/ &&
  ${CMAKE_COMMAND} -E copy status-syncing.icns ${TARGET_DIR}/Contents/Resources/ &&
  ${CMAKE_COMMAND} -E copy status-unknown.icns ${TARGET_DIR}/Contents/Resources/ &&
  ${CODESIGN} --force --sign ${CODESIGN_CERTIFICATE} --entitlements ${CMAKE_CURRENT_SOURCE_DIR}/seafile-fsplugin.entitlements ${TARGET_DIR}
  )
