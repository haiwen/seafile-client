CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)

PROJECT(seafile-client)
SET(SEAFILE_CLIENT_VERSION_MAJOR 6)
SET(SEAFILE_CLIENT_VERSION_MINOR 1)
SET(SEAFILE_CLIENT_VERSION_PATCH 8)
SET(PROJECT_VERSION "${SEAFILE_CLIENT_VERSION_MAJOR}.${SEAFILE_CLIENT_VERSION_MINOR}.${SEAFILE_CLIENT_VERSION_PATCH}")
ADD_DEFINITIONS(-DSEAFILE_CLIENT_VERSION=${PROJECT_VERSION})
INCLUDE(FindPkgConfig)

## Build crash repoter on release build as default
IF (NOT (${CMAKE_BUILD_TYPE} MATCHES Release))
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

# CMake's builtin UNIX variable is true on Mac/Linux/BSD, but sometimes we want
# to detect for LINUX.
IF (UNIX AND NOT APPLE)
  SET(LINUX TRUE)
ENDIF()

IF (APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.7")
ENDIF()

FIND_PROGRAM(git_executable NAMES git git.exe git.cmd)
IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git AND NOT (${CMAKE_BUILD_TYPE} MATCHES Release))
  EXECUTE_PROCESS(COMMAND
    ${git_executable} rev-list HEAD --count
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE SEAFILE_CLIENT_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE )
  ADD_DEFINITIONS(-DSEAFILE_CLIENT_REVISION=${SEAFILE_CLIENT_REVISION})
ENDIF()

SET(PATH_TO_BREAKPAD_ROOT CACHE "Path to breakpad's root" "")

OPTION(BUILD_TESTING "Build Test" OFF)

OPTION(BUILD_DOCS "Build Documents" OFF)

OPTION(BUILD_SHIBBOLETH_SUPPORT "Build Shibboleth support" OFF)

option(BUILD_ENABLE_WARNINGS "Enable compiler warnings." ON)

option(BUILD_SPARKLE_SUPPORT "Build Sparkle support" OFF)
IF (BUILD_SPARKLE_SUPPORT)
    ADD_DEFINITIONS(-DHAVE_SPARKLE_SUPPORT)
    IF (LINUX)
      message(FATAL_ERROR "Sparkle Support is not available for linux." )
    ENDIF()
    SET(platform_specific_moc_headers ${platform_specific_moc_headers} src/auto-update-service.h)
    SET(platform_specific_sources ${platform_specific_sources} src/auto-update-service.cpp)
    IF (NOT (${CMAKE_BUILD_TYPE} MATCHES Release))
        ADD_DEFINITIONS(-DSEAFILE_SPARKLE_DEBUG)
    ENDIF()
ENDIF()

MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")

## build in PIC mode
IF (NOT WIN32)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF()

## Setup warnings ouput if enabled
## but don't warning on unsupported warnings
if(BUILD_ENABLE_WARNINGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wsign-compare -Wno-long-long -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wsign-compare -Wno-long-long -Wno-unused-parameter -Woverloaded-virtual")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-warning-option -Wno-inconsistent-missing-override")
  endif()
endif()

## color diagnostics fix
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  execute_process(COMMAND ${CMAKE_CXX_COMPILER}
    -dumpversion OUTPUT_VARIABLE GCC_VERSION)
  if(GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
  endif()
endif()

IF (WIN32)
    CONFIGURE_FILE(
      ${CMAKE_SOURCE_DIR}/seafile-applet.rc.in
      ${CMAKE_BINARY_DIR}/seafile-applet.rc
    )
    SET(EXTRA_LIBS ${EXTRA_LIBS} psapi ws2_32 shlwapi mpr)
    SET(EXTRA_SOURCES ${EXTRA_SOURCES} ${CMAKE_BINARY_DIR}/seafile-applet.rc)
    IF (${CMAKE_BUILD_TYPE} MATCHES Release)
      SET(GUI_TYPE WIN32)
    ENDIF()
    SET(platform_specific_moc_headers ${platform_specific_moc_headers} src/ext-handler.h)
    SET(platform_specific_sources ${platform_specific_sources} src/ext-handler.cpp)
    IF (BUILD_SPARKLE_SUPPORT)
      IF(NOT EXISTS "${CMAKE_SOURCE_DIR}/WinSparkle.lib")
        message(FATAL_ERROR "File ${CMAKE_SOURCE_DIR}/WinSparkle.lib not found in current directory. Please setup winsparkle correctly." )
      ENDIF()
      SET(SPARKLE_LIBS ${CMAKE_SOURCE_DIR}/WinSparkle.lib)
    ENDIF()

ELSEIF (LINUX)
    INCLUDE_DIRECTORIES(${QT_QTDBUS_INCLUDE_DIR})
    LINK_DIRECTORIES(${QT_QTDBUS_LIBRARIES})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${QT_QTDBUS_LIBRARIES})
ELSEIF (APPLE)
    IF (BUILD_SPARKLE_SUPPORT)
        SET(platform_specific_sources ${platform_specific_sources} src/mac-sparkle-support.mm)
        SET_SOURCE_FILES_PROPERTIES(src/mac-sparkle-support.mm
          PROPERTIES COMPILE_FLAGS -fobjc-arc)

        SET(SPARKLE_FRAMEWORK /usr/local/Sparkle.framework)
        IF(NOT EXISTS "${SPARKLE_FRAMEWORK}")
          message(FATAL_ERROR "File ${SPARKLE_FRAMEWORK} not found. Please setup it correctly.")
        ENDIF()
        INCLUDE_DIRECTORIES(${SPARKLE_FRAMEWORK}/Headers)
        SET(EXTRA_LIBS ${EXTRA_LIBS} ${SPARKLE_FRAMEWORK})
    ENDIF()

    SET(platform_specific_sources ${platform_specific_sources} src/application.cpp)
    ## Enforce ARC for this file, since ARC is only supported after the objc
    ## runtime changes in os x 10.10
    SET_SOURCE_FILES_PROPERTIES(src/utils/utils-mac.mm
      PROPERTIES COMPILE_FLAGS -fobjc-arc)

    FIND_LIBRARY(COCOA_LIBRARY Cocoa)
    MARK_AS_ADVANCED (COCOA_LIBRARY)
    FIND_LIBRARY(Sec_LIBRARY Security)
    MARK_AS_ADVANCED (Sec_LIBRARY)
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${COCOA_LIBRARY} ${Sec_LIBRARY})
ENDIF()

IF(APPLE)
    ADD_DEFINITIONS(-DHAVE_FINDER_SYNC_SUPPORT)
    SET(platform_specific_moc_headers ${platform_specific_moc_headers} src/finder-sync/finder-sync-host.h)
    SET(platform_specific_sources ${platform_specific_sources}
        src/finder-sync/finder-sync.cpp src/finder-sync/finder-sync-listener.mm
        src/finder-sync/finder-sync-host.cpp)
    SET_SOURCE_FILES_PROPERTIES(src/finder-sync/finder-sync-listener.mm
        PROPERTIES COMPILE_FLAGS -fobjc-arc)
ENDIF()

####################
###### BEGIN: QT configuration
####################
SET(QT_VERSION_MAJOR 5)

# Qt 5.6.0 removes QtWebKit, we need to use QtWebEnigne. First we detect the
# current qt version, then use proper name based on the version. However we
# have to keep using QtWebKit on windows because QtWebEngine can't be
# compiled in msys2/mingw (QtWebEnigne is based on chrome, which has to be
# compiled with MSVC.)
FIND_PROGRAM(qmake_executable NAMES qmake qmake.exe)
EXECUTE_PROCESS(COMMAND
    bash -c "${qmake_executable} --version | grep -iE '^using qt version [0-9.]+' | awk '{print $4}'"
    OUTPUT_VARIABLE DETECTED_QT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
MESSAGE("qt5 version: ${DETECTED_QT_VERSION}")
IF(WIN32 OR DETECTED_QT_VERSION VERSION_LESS 5.6.0)
    ADD_DEFINITIONS(-DSEAFILE_USE_WEBKIT)
    SET(WEBKIT_NAME "WebKit")
    SET(WEBKIT_WIDGETS_NAME "WebKitWidgets")
ELSE()
    SET(WEBKIT_NAME "WebEngine")
    SET(WEBENGINE_CORE "WebEngineCore")
    SET(WEBKIT_WIDGETS_NAME "WebEngineWidgets")
    SET(SHIB_EXTRA_HEADER "src/shib/shib-helper.h")
ENDIF()

SET(USE_QT_LIBRARIES
    Core Gui Widgets LinguistTools Network
    )
IF (BUILD_SHIBBOLETH_SUPPORT)
    SET(USE_QT_LIBRARIES ${USE_QT_LIBRARIES} ${WEBENGINE_CORE} ${WEBKIT_NAME} ${WEBKIT_WIDGETS_NAME})
    ADD_DEFINITIONS(-DHAVE_SHIBBOLETH_SUPPORT)
ENDIF()

IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(USE_QT_LIBRARIES ${USE_QT_LIBRARIES} "DBus")
ENDIF()

FIND_PACKAGE(Qt5 REQUIRED ${USE_QT_LIBRARIES})
SET(QT_LUPDATE_EXECUTABLE ${Qt5_LUPDATE_EXECUTABLE})
# from QT 5.4.0 Changelog
# The Qt binary packages are now configured with C++11 enabled.
# this requires your gcc compiler newer than 4.8.1 or clang newer than 3.3

if (CYGWIN OR MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# for OS X, we requires libc++ instead
if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
    SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
endif()

ADD_DEFINITIONS(-DQUAZIP_STATIC)

# MOC FILES
SET(moc_headers
  src/seafile-applet.h
  src/account-mgr.h
  src/configurator.h
  src/daemon-mgr.h
  src/auto-login-service.h
  src/repo-service.h
  src/repo-service-helper.h
  src/events-service.h
  src/avatar-service.h
  src/open-local-helper.h
  src/server-status-service.h
  src/account-info-service.h
  src/customization-service.h
  src/message-listener.h
  src/network-mgr.h
  src/settings-mgr.h
  src/traynotificationwidget.h
  src/traynotificationmanager.h
  src/log-uploader.h
  src/api/api-client.h
  src/api/api-request.h
  src/api/requests.h
  src/rpc/rpc-client.h
  src/ui/main-window.h
  src/ui/init-seafile-dialog.h
  src/ui/login-dialog.h
  src/ui/account-settings-dialog.h
  src/ui/check-repo-root-perm-dialog.h
  src/ui/create-repo-dialog.h
  src/ui/repo-detail-dialog.h
  src/ui/settings-dialog.h
  src/ui/download-repo-dialog.h
  src/ui/cloud-view.h
  src/ui/tray-icon.h
  src/ui/about-dialog.h
  src/ui/repo-tree-model.h
  src/ui/repo-tree-view.h
  src/ui/repo-item-delegate.h
  src/ui/clone-tasks-dialog.h
  src/ui/clone-tasks-table-model.h
  src/ui/clone-tasks-table-view.h
  src/ui/server-status-dialog.h
  src/ui/search-tab.h
  src/ui/search-tab-items.h
  src/ui/init-vdrive-dialog.h
  src/ui/uninstall-helper-dialog.h
  src/ui/ssl-confirm-dialog.h
  src/ui/private-share-dialog.h
  src/ui/user-name-completer.h
  src/ui/account-view.h
  src/ui/seafile-tab-widget.h
  src/ui/tab-view.h
  src/ui/loading-view.h
  src/ui/logout-view.h
  src/ui/repos-tab.h
  src/ui/starred-files-tab.h
  src/ui/starred-files-list-view.h
  src/ui/starred-files-list-model.h
  src/ui/starred-file-item-delegate.h
  src/ui/activities-tab.h
  src/ui/events-list-view.h
  src/ui/event-details-dialog.h
  src/ui/event-details-tree.h
  src/ui/set-repo-password-dialog.h
  src/ui/sync-errors-dialog.h
  src/ui/search-bar.h
  src/ui/two-factor-dialog.h
  src/filebrowser/file-browser-manager.h
  src/filebrowser/file-browser-dialog.h
  src/filebrowser/file-browser-requests.h
  src/filebrowser/file-browser-search-tab.h
  src/filebrowser/file-table.h
  src/filebrowser/data-mgr.h
  src/filebrowser/tasks.h
  src/filebrowser/reliable-upload.h
  src/filebrowser/progress-dialog.h
  src/filebrowser/sharedlink-dialog.h
  src/filebrowser/seafilelink-dialog.h
  src/filebrowser/auto-update-mgr.h
  src/filebrowser/transfer-mgr.h
  src/filebrowser/thumbnail-service.h
  third_party/QtAwesome/QtAwesome.h
  third_party/quazip/quazipfile.h
  ${platform_specific_moc_headers}
)

IF (APPLE)
  SET(moc_headers ${moc_headers} src/application.h)
ENDIF()

IF (BUILD_SHIBBOLETH_SUPPORT)
  SET(moc_headers ${moc_headers} src/shib/shib-login-dialog.h ${SHIB_EXTRA_HEADER})
ENDIF()

# UI FILES
SET(ui_files
  ui/about-dialog.ui
  ui/login-dialog.ui
  ui/account-settings-dialog.ui
  ui/create-repo-dialog.ui
  ui/repo-detail-dialog.ui
  ui/settings-dialog.ui
  ui/download-repo-dialog.ui
  ui/init-seafile-dialog.ui
  ui/cloud-view.ui
  ui/clone-tasks-dialog.ui
  ui/server-status-dialog.ui
  ui/init-vdrive-dialog.ui
  ui/uninstall-helper-dialog.ui
  ui/ssl-confirm-dialog.ui
  ui/private-share-dialog.ui
  ui/account-view.ui
  ui/two-factor-dialog.ui
  ui/set-repo-password-dialog.ui
  ${platform_specific_ui_files}
)

# RESOURCES
SET(qrc_files
  seafile-client.qrc
  third_party/QtAwesome/QtAwesome.qrc
)

QT5_WRAP_UI(ui_output ${ui_files})

# meta object compliation(moc)
QT5_WRAP_CPP(moc_output ${moc_headers})

# resources files
QT5_ADD_RESOURCES(resources_ouput ${qrc_files})

# MESSAGE("moc output: ${moc_output}")
# MESSAGE("ui output: ${ui_output}")

####################
###### END: QT configuration
####################

####################
###### BEGIN: pthread support is required explicitly on linux
####################

IF(NOT WIN32 AND NOT APPLE)
  SET(CMAKE_THREAD_PREFER_PTHREAD ON)
  INCLUDE(FindThreads)
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS_INIT})
ENDIF(NOT WIN32 AND NOT APPLE)

####################
###### BEGIN: other libraries configuration
####################
FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(SQLITE3 REQUIRED sqlite3>=3.0.0)

PKG_CHECK_MODULES(JANSSON REQUIRED jansson>=2.0)

PKG_CHECK_MODULES(LIBCCNET REQUIRED libccnet>=1.3)

PKG_CHECK_MODULES(LIBSEARPC REQUIRED libsearpc>=1.0)

PKG_CHECK_MODULES(OPENSSL REQUIRED openssl>=0.98)

PKG_CHECK_MODULES(LIBSEAFILE REQUIRED libseafile>=1.7)

PKG_CHECK_MODULES(LIBEVENT REQUIRED libevent>=2.0)

PKG_CHECK_MODULES(ZLIB REQUIRED zlib>=1.2)
####################
###### END: other libraries configuration
####################


# c/cpp sources
SET(seafile_client_sources
  src/i18n.cpp
  src/main.cpp
  src/seafile-applet.cpp
  src/account.cpp
  src/account-mgr.cpp
  src/ccnet-init.cpp
  src/daemon-mgr.cpp
  src/configurator.cpp
  src/open-local-helper.cpp
  src/message-listener.cpp
  src/network-mgr.cpp
  src/auto-login-service.cpp
  src/repo-service.cpp
  src/repo-service-helper.cpp
  src/events-service.cpp
  src/server-status-service.cpp
  src/account-info-service.cpp
  src/customization-service.cpp
  src/shared-application.cpp
  src/avatar-service.cpp
  src/settings-mgr.cpp
  src/traynotificationwidget.cpp
  src/traynotificationmanager.cpp
  src/certs-mgr.cpp
  src/log-uploader.cpp
  src/api/api-client.cpp
  src/api/api-request.cpp
  src/api/api-error.cpp
  src/api/requests.cpp
  src/api/server-repo.cpp
  src/api/starred-file.cpp
  src/api/event.cpp
  src/api/commit-details.cpp
  src/api/contact-share-info.cpp
  src/rpc/rpc-client.cpp
  src/rpc/local-repo.cpp
  src/rpc/clone-task.cpp
  src/rpc/sync-error.cpp
  src/ui/about-dialog.cpp
  src/ui/main-window.cpp
  src/ui/init-seafile-dialog.cpp
  src/ui/login-dialog.cpp
  src/ui/account-settings-dialog.cpp
  src/ui/check-repo-root-perm-dialog.cpp
  src/ui/repo-detail-dialog.cpp
  src/ui/settings-dialog.cpp
  src/ui/create-repo-dialog.cpp
  src/ui/download-repo-dialog.cpp
  src/ui/tray-icon.cpp
  src/ui/cloud-view.cpp
  src/utils/uninstall-helpers.cpp
  src/ui/repo-item.cpp
  src/ui/repo-tree-model.cpp
  src/ui/repo-tree-view.cpp
  src/ui/repo-item-delegate.cpp
  src/ui/clone-tasks-dialog.cpp
  src/ui/clone-tasks-table-model.cpp
  src/ui/clone-tasks-table-view.cpp
  src/ui/server-status-dialog.cpp
  src/ui/init-vdrive-dialog.cpp
  src/ui/uninstall-helper-dialog.cpp
  src/ui/search-tab.cpp
  src/ui/search-tab-items.cpp
  src/ui/ssl-confirm-dialog.cpp
  src/ui/private-share-dialog.cpp
  src/ui/user-name-completer.cpp
  src/ui/proxy-style.cpp
  src/ui/account-view.cpp
  src/ui/seafile-tab-widget.cpp
  src/ui/tab-view.cpp
  src/ui/loading-view.cpp
  src/ui/logout-view.cpp
  src/ui/repos-tab.cpp
  src/ui/starred-files-tab.cpp
  src/ui/starred-files-list-view.cpp
  src/ui/starred-files-list-model.cpp
  src/ui/starred-file-item.cpp
  src/ui/starred-file-item-delegate.cpp
  src/ui/activities-tab.cpp
  src/ui/events-list-view.cpp
  src/ui/event-details-dialog.cpp
  src/ui/event-details-tree.cpp
  src/ui/set-repo-password-dialog.cpp
  src/ui/sync-errors-dialog.cpp
  src/ui/search-bar.cpp
  src/ui/two-factor-dialog.cpp
  src/filebrowser/file-browser-manager.cpp
  src/filebrowser/file-browser-dialog.cpp
  src/filebrowser/file-browser-requests.cpp
  src/filebrowser/file-browser-search-tab.cpp
  src/filebrowser/data-mgr.cpp
  src/filebrowser/data-cache.cpp
  src/filebrowser/file-table.cpp
  src/filebrowser/seaf-dirent.cpp
  src/filebrowser/reliable-upload.cpp
  src/filebrowser/tasks.cpp
  src/filebrowser/progress-dialog.cpp
  src/filebrowser/sharedlink-dialog.cpp
  src/filebrowser/seafilelink-dialog.cpp
  src/filebrowser/auto-update-mgr.cpp
  src/filebrowser/transfer-mgr.cpp
  src/filebrowser/thumbnail-service.cpp
  third_party/QtAwesome/QtAwesome.cpp
  ${platform_specific_sources}
)

IF (BUILD_SHIBBOLETH_SUPPORT)
  SET(seafile_client_sources ${seafile_client_sources} src/shib/shib-login-dialog.cpp)
ENDIF()

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/QtAwesome
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quazip
  )

FOREACH(USE_QT_LIBRARY ${USE_QT_LIBRARIES})
INCLUDE_DIRECTORIES(
${Qt5${USE_QT_LIBRARY}_INCLUDE_DIRS}
)
ENDFOREACH()

INCLUDE_DIRECTORIES(
  ${OPENSSL_INCLUDE_DIRS}
  ${LIBEVENT_INCLUDE_DIRS}
  ${SQLITE3_INCLUDE_DIRS}
  ${JANSSON_INCLUDE_DIRS}
  ${LIBSEARPC_INCLUDE_DIRS}
  ${LIBCCNET_INCLUDE_DIRS}
  ${LIBSEAFILE_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
  ${QT_LIBRARY_DIR}
  ${OPENSSL_LIBRARY_DIRS}
  ${LIBEVENT_LIBRARY_DIRS}
  ${LIBCCNET_LIBRARY_DIRS}
  ${LIBSEAFILE_LIBRARY_DIRS}
  ${LIBSEARPC_LIBRARY_DIRS}
  ${SQLITE3_LIBRARRY_DIRS}
  ${JANSSON_LIBRARRY_DIRS}
  ${ZLIB_LIBRARRY_DIRS}
)

####################
###### begin: lib
####################

# Helper Function to Add Seafile-Client Library
# Usage:
# ADD_SC_LIBRARY (name sources
#     MOC_HEADERS moc_headers
#     UI_FILES ui_files
#     DEPENDS part
#     LINK_LIBS libs
#     )
FUNCTION(ADD_SC_LIBRARY name)
    MESSAGE(STATUS "Found internal library: ${name}")
    CMAKE_PARSE_ARGUMENTS(ARG
      ""
      ""
      "MOC_HEADERS;UI_FILES;DEPENDS;LINK_LIBS"
      ${ARGN})
    IF(ARG_MOC_HEADERS)
        QT5_WRAP_CPP(MOC_OUTPUT ${ARG_MOC_HEADERS})
    ENDIF()
    IF(ARG_UI_FILES)
        QT5_WRAP_UI(UI_OUTPUT ${ARG_UI_FILES})
    ENDIF()

    ADD_LIBRARY(${name} STATIC ${ARG_UNPARSED_ARGUMENTS}
      ${MOC_OUTPUT} ${UI_OUTPUT})

    TARGET_LINK_LIBRARIES(${name} ${QT_LIBRARIES}
        ${SQLITE3_LIBRARIES} ${JANSSON_LIBRARIES}
        ${EXTRA_LIBS} -lglib-2.0 ${ARG_LINK_LIBS} ${ZLIB_LIBRARIES})
ENDFUNCTION(ADD_SC_LIBRARY)

# library utils
LIST(APPEND utils_sources
    src/utils/utils.cpp
    src/utils/rsa.cpp
    src/utils/api-utils.cpp
    src/utils/paint-utils.cpp
    src/utils/file-utils.cpp
    src/utils/translate-commit-desc.cpp
    src/utils/json-utils.cpp
    src/utils/log.c
    src/utils/stl.cpp
    )
IF (WIN32)
    LIST(APPEND utils_sources
        src/utils/process-win.cpp
        src/utils/registry.cpp
        src/utils/utils-win.cpp
        )
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    LIST(APPEND utils_sources
        src/utils/process-linux.cpp
        )
ELSEIF(APPLE)
    LIST(APPEND utils_sources
        src/utils/process-mac.cpp
        src/utils/utils-mac.mm)
ENDIF()

ADD_SC_LIBRARY(utils ${utils_sources})

# library quazip
LIST(APPEND quazip_sources
    third_party/quazip/JlCompress.cpp
    third_party/quazip/quazip.cpp
    third_party/quazip/quazipfile.cpp
    third_party/quazip/quazipfileinfo.cpp
    third_party/quazip/quaziodevice.cpp
    third_party/quazip/quagzipfile.cpp
    third_party/quazip/quazipnewinfo.cpp
    third_party/quazip/quaadler32.cpp
    third_party/quazip/quazipdir.cpp
    third_party/quazip/quacrc32.cpp
    third_party/quazip/qioapi.cpp
    third_party/quazip/zip.c
    third_party/quazip/unzip.c
    )

ADD_SC_LIBRARY(quazip ${quazip_sources})

SET(SC_LIBS utils quazip)
#SET(SC_LIBS utils)

####################
###### end: lib
####################

####################
###### start: translations
####################

SET(SEAFILE_TRANSLATE_SOURCES ${seafile_client_sources} ${utils_sources} ${moc_output} ${ui_output})
SET(LANGUAGES
  ca
  de_DE
  en
  es
  es_AR
  es_MX
  fr_FR
  he_IL
  hu_HU
  is
  it
  ko_KR
  nl_BE
  pl_PL
  pt_BR
  pt_PT
  ru
  sk_SK
  uk
  zh_CN
  zh_TW
  tr
  nl_NL
  lv
  ja
  sv
  cs_CZ
  el_GR
  nb_NO
  )

SET(LANGUAGE_TS_FILES)
SET(SEAFILE_TS_TARGETS)
FOREACH(LANGUAGE ${LANGUAGES})
  SET(TS_FILE "${PROJECT_SOURCE_DIR}/i18n/seafile_${LANGUAGE}.ts")
  SET(LANGUAGE_TS_FILES ${LANGUAGE_TS_FILES} ${TS_FILE})
  SET_SOURCE_FILES_PROPERTIES(${TS_FILE} PROPERTIES OUTPUT_LOCATION "${PROJECT_SOURCE_DIR}/i18n")

  SET(TS_TARGET "${LANGUAGE}_ts")
  ADD_CUSTOM_TARGET(${TS_TARGET}
    COMMAND ${QT_LUPDATE_EXECUTABLE} ${SEAFILE_TRANSLATE_SOURCES}
        -ts "${PROJECT_SOURCE_DIR}/i18n/seafile_${LANGUAGE}.ts"
        -I "${PROJECT_SOURCE_DIR}/src"
        -locations none
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

  SET(SEAFILE_TS_TARGETS ${SEAFILE_TS_TARGETS} ${TS_TARGET})

ENDFOREACH(LANGUAGE ${LANGUAGES})

QT5_ADD_TRANSLATION(qm_files ${LANGUAGE_TS_FILES})

ADD_CUSTOM_TARGET(update-ts DEPENDS ${SEAFILE_TS_TARGETS})

####################
###### end: translations
####################

###################
##### begin: doxygen
##################
FIND_PACKAGE(Doxygen)

IF (DOXYGEN_FOUND)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  ADD_CUSTOM_TARGET(doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating doxygen documentation." VERBATIM)

  IF(BUILD_DOCS)
    ADD_CUSTOM_TARGET(doxygen ALL)
  ENDIF()
ENDIF()

###################
##### end: doxygen
##################

####################
###### begin: crash reporter
####################

# if you haven't installed it, please visit
# https://code.google.com/p/google-breakpad/ to get a copy of it
# or run the script build_breakpad.sh under the scripts directory

## set up our crash reporter: breakpad

IF (PATH_TO_BREAKPAD_ROOT)
    FIND_LIBRARY(BREAKPAD_LIBRARY breakpad
      PATHS ${PATH_TO_BREAKPAD_ROOT}/out/Debug_Base
      PATHS ${PATH_TO_BREAKPAD_ROOT}/out/Default/obj
      NO_DEFAULT_PATH)
    FIND_LIBRARY(BREAKPAD_UTILITIES_LIBRARY breakpad_utilities
      PATHS ${PATH_TO_BREAKPAD_ROOT}/out/Debug_Base
      PATHS ${PATH_TO_BREAKPAD_ROOT}/out/Default/obj
      NO_DEFAULT_PATH)
    MESSAGE(STATUS "Found library: ${BREAKPAD_LIBRARY}")
    MESSAGE(STATUS "Found library: ${BREAKPAD_UTILITIES_LIBRARY}")

    SET(seafile_client_sources ${seafile_client_sources} src/crash-handler.cpp)
    INCLUDE_DIRECTORIES()
    ADD_DEFINITIONS(-DSEAFILE_CLIENT_HAS_CRASH_REPORTER)
    SET_SOURCE_FILES_PROPERTIES(src/crash-handler.cpp
        PROPERTIES COMPILE_FLAGS "-I${PATH_TO_BREAKPAD_ROOT}/src")
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${BREAKPAD_LIBRARY} ${BREAKPAD_UTILITIES_LIBRARY})
ENDIF()

####################
###### end: crash reporter
####################

####################
###### start: freedesktop files
####################

if(NOT WIN32)
install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/data/icons/16x16
  ${CMAKE_SOURCE_DIR}/data/icons/22x22
  ${CMAKE_SOURCE_DIR}/data/icons/24x24
  ${CMAKE_SOURCE_DIR}/data/icons/32x32
  ${CMAKE_SOURCE_DIR}/data/icons/48x48
  ${CMAKE_SOURCE_DIR}/data/icons/128x128
  ${CMAKE_SOURCE_DIR}/data/icons/scalable
  DESTINATION share/icons/hicolor
)

install(FILES
  ${CMAKE_SOURCE_DIR}/data/seafile.desktop
  DESTINATION share/applications
)

install(FILES
  ${CMAKE_SOURCE_DIR}/data/icons/128x128/apps/seafile.png
  DESTINATION share/pixmaps
)

endif()

####################
###### end: freedesktop files
####################

ADD_EXECUTABLE(seafile-applet ${GUI_TYPE}
  ${seafile_client_sources}
  ${moc_output}
  ${ui_output}
  ${resources_ouput}
  ${EXTRA_SOURCES}
)

INSTALL(TARGETS seafile-applet DESTINATION bin)

TARGET_LINK_LIBRARIES(seafile-applet
  ${SPARKLE_LIBS}
  ${SC_LIBS}
  ${QT_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${LIBEVENT_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${JANSSON_LIBRARIES}
  ${LIBSEARPC_LIBRARIES}
  ${LIBCCNET_LIBRARIES}
  ${LIBSEAFILE_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${EXTRA_LIBS}
)

target_link_libraries(seafile-applet Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
IF (BUILD_SHIBBOLETH_SUPPORT)
target_link_libraries(seafile-applet Qt5::WebEngine Qt5::WebEngineWidgets)
ENDIF()

## QtBus
IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
  target_link_libraries(seafile-applet Qt5::DBus)
ENDIF()

### Xcode-related, build as a osx bundle
IF(CMAKE_GENERATOR STREQUAL Xcode)
  ADD_DEFINITIONS(-DXCODE_APP)
  SET_TARGET_PROPERTIES(seafile-applet PROPERTIES
    MACOSX_BUNDLE true
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  )
  FIND_PROGRAM(ccnet ccnet)
  FIND_PROGRAM(seaf-daemon seaf-daemon)
  SET(RESOURCES_DIR ${CMAKE_CURRENT_BINARY_DIR}/\${CONFIGURATION}/seafile-applet.app/Contents/Resources)
  ADD_CUSTOM_COMMAND(TARGET seafile-applet
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCES_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy seafile.icns ${RESOURCES_DIR}/.
      COMMAND ${CMAKE_COMMAND} -E copy ${ccnet} ${RESOURCES_DIR}/.
      COMMAND ${CMAKE_COMMAND} -E copy ${seaf-daemon} ${RESOURCES_DIR}/.
  )
  SET(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
  SET(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
  SET(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_PASCAL_STRINGS "NO")
  SET(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
ENDIF()

SET(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})
ADD_CUSTOM_TARGET(dist
    COMMAND ${git_executable} archive -v --prefix=${ARCHIVE_NAME}/ HEAD
        | gzip > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

### Test related
IF (BUILD_TESTING)
    MACRO(ADD_QTEST testname)
        QT5_WRAP_CPP(${testname}_MOCHEADER tests/${testname}.h)

        SET(${testname}_SRCS tests/${testname}.cpp ${${testname}_MOCHEADER})

        ADD_EXECUTABLE(${testname} ${${testname}_SRCS})

        TARGET_LINK_LIBRARIES(${testname} ${QT_LIBRARIES}
                ${QTESTLIB} ${SQLITE3_LIBRARIES} ${JANSSON_LIBRARIES}
                ${EXTRA_LIBS} -lglib-2.0 ${SC_LIBS} ${ZLIB_LIBRARIES})
        SET_TARGET_PROPERTIES(${testname} PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests )

        INCLUDE_DIRECTORIES(${Qt5Test_INCLUDE_DIRS})

        QT5_USE_MODULES(${testname} ${USE_QT_LIBRARIES} Test)

        ADD_TEST(${testname} ${CMAKE_CURRENT_BINARY_DIR}/tests/${testname})

    ENDMACRO(ADD_QTEST)

    ENABLE_TESTING()
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/tests)
    ADD_QTEST(test_server-info)
    ADD_QTEST(test_utils)
    ADD_QTEST(test_file-utils)
    ADD_QTEST(test_stl)
ENDIF()
