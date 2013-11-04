CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(seafile-client)
SET(PROJECT_VERSION "2.0.6")
ADD_DEFINITIONS(-DSEAFILE_CLIENT_VERSION=2.0.6)
INCLUDE(FindPkgConfig)

SET(BRAND \"Seafile\")
ADD_DEFINITIONS(-DSEAFILE_CLIENT_BRAND=${BRAND})

IF (NOT (${CMAKE_BUILD_TYPE} MATCHES Release))
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")

IF (MSYS)
  SET(EXTRA_LIBS ${EXTRA_LIBS} psapi ws2_32)

  # SET(CMAKE_RC_COMPILER_INIT windres)
  # ENABLE_LANGUAGE(RC)
  # SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")

  SET(EXTRA_SOURCES ${EXTRA_SOURCES} seafile-applet.rc)
ENDIF (MSYS)


####################
###### BEGIN: QT configuration
####################
FIND_PACKAGE(Qt4 REQUIRED QtCore QtGui QtNetwork)
INCLUDE(${QT_USE_FILE})


# MOC FILES
SET(moc_headers
  src/seafile-applet.h
  src/account-mgr.h
  src/configurator.h
  src/daemon-mgr.h
  src/message-listener.h
  src/settings-mgr.h
  src/traynotificationwidget.h
  src/traynotificationmanager.h
  src/api/api-client.h
  src/api/api-request.h
  src/api/requests.h
  src/rpc/rpc-client.h
  src/ui/main-window.h
  src/ui/init-seafile-dialog.h
  src/ui/login-dialog.h
  src/ui/welcome-dialog.h
  src/ui/create-repo-dialog.h
  src/ui/repo-detail-dialog.h
  src/ui/settings-dialog.h
  src/ui/download-repo-dialog.h
  src/ui/cloud-view.h
  src/ui/tray-icon.h
  src/ui/repo-tree-model.h
  src/ui/repo-tree-view.h
  src/ui/repo-item-delegate.h
  src/ui/clone-tasks-dialog.h
  src/ui/clone-tasks-table-model.h
  src/ui/clone-tasks-table-view.h
  src/ui/server-status-dialog.h
  third_party/QtAwesome/QtAwesome.h
)

IF (APPLE)
  SET(moc_headers ${moc_headers} src/application.h)
ENDIF()

# UI FILES
SET(ui_files
  ui/login-dialog.ui
  ui/welcome-dialog.ui
  ui/create-repo-dialog.ui
  ui/repo-detail-dialog.ui
  ui/settings-dialog.ui
  ui/download-repo-dialog.ui
  ui/init-seafile-dialog.ui
  ui/cloud-view.ui
  ui/clone-tasks-dialog.ui
  ui/server-status-dialog.ui
)

# RESOURCES
SET(qrc_files
  seafile-client.qrc
  third_party/QtAwesome/QtAwesome.qrc
)

# preprocess ui files
QT4_WRAP_UI(ui_output ${ui_files})

# meta object compliation(moc)
QT4_WRAP_CPP(moc_output ${moc_headers})

# resources files
QT4_ADD_RESOURCES(resources_ouput ${qrc_files})

# MESSAGE("moc output: ${moc_output}")
# MESSAGE("ui output: ${ui_output}")

####################
###### END: QT configuration
####################

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
####################
###### END: other libraries configuration
####################

IF (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET(platform_specific_sources
      src/utils/process-win.cpp
    )
    SET(GUI_TYPE WIN32)
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(platform_specific_sources
      src/utils/process-linux.cpp
    )
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(platform_specific_sources
      src/utils/process-mac.cpp src/application.cpp
    )

    FIND_LIBRARY(CARBON_LIBRARY Carbon)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa )
    MARK_AS_ADVANCED (CARBON_LIBRARY COCOA_LIBRARY)
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${CARBON_LIBRARY} ${COCOA_LIBRARY})
endif()


# c/cpp sources
SET(seafile_client_sources
  src/main.cpp
  src/seafile-applet.cpp
  src/account-mgr.cpp
  src/ccnet-init.cpp
  src/daemon-mgr.cpp
  src/configurator.cpp
  src/message-listener.cpp
  src/settings-mgr.cpp
  src/traynotificationwidget.cpp
  src/traynotificationmanager.cpp
  src/api/api-client.cpp
  src/api/api-request.cpp
  src/api/requests.cpp
  src/api/server-repo.cpp
  src/rpc/rpc-client.cpp
  src/rpc/local-repo.cpp
  src/rpc/clone-task.cpp
  src/ui/main-window.cpp
  src/ui/init-seafile-dialog.cpp
  src/ui/login-dialog.cpp
  src/ui/welcome-dialog.cpp
  src/ui/repo-detail-dialog.cpp
  src/ui/settings-dialog.cpp
  src/ui/create-repo-dialog.cpp
  src/ui/download-repo-dialog.cpp
  src/ui/tray-icon.cpp
  src/ui/cloud-view.cpp
  src/utils/rsa.cpp
  src/utils/utils.cpp
  src/utils/log.c
  src/ui/repo-item.cpp
  src/ui/repo-tree-model.cpp
  src/ui/repo-tree-view.cpp
  src/ui/repo-item-delegate.cpp
  src/ui/clone-tasks-dialog.cpp
  src/ui/clone-tasks-table-model.cpp
  src/ui/clone-tasks-table-view.cpp
  src/ui/server-status-dialog.cpp
  third_party/QtAwesome/QtAwesome.cpp
  ${platform_specific_sources}
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/third_party/QtAwesome
  ${QT_INCLUDE_DIR}
  ${QT_QTNETWORK_INCLUDE_DIR}
  ${OPEN_SSL_INCLUDE_DIRS}
  ${SQLITE3_INCLUDE_DIRS}
  ${JANSSON_INCLUDE_DIRS}
  ${LIBSEARPC_INCLUDE_DIRS}
  ${LIBCCNET_INCLUDE_DIRS}
  ${LIBSEAFILE_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
  ${QT_LIBRARY_DIR}
  ${LIBCCNET_LIBRARY_DIRS}
  ${LIBSEAFILE_LIBRARY_DIRS}
  ${LIBSEARPC_LIBRARY_DIRS}
  ${SQLITE3_LIBRARRY_DIRS}
  ${JANSSON_LIBRARRY_DIRS}
)

####################
###### start: translations
####################

SET(SEAFILE_TRANSLATE_SOURCES ${seafile_client_sources} ${moc_output} ${ui_output})
SET(LANGUAGES en de_DE fr_FR hu_HU zh_CN)

SET(LANGUAGE_TS_FILES)
SET(SEAFILE_TS_TARGETS)
FOREACH(LANGUAGE ${LANGUAGES})
  SET(TS_FILE "${PROJECT_SOURCE_DIR}/i18n/seafile_${LANGUAGE}.ts")
  SET(LANGUAGE_TS_FILES ${LANGUAGE_TS_FILES} ${TS_FILE})
  SET_SOURCE_FILES_PROPERTIES(${TS_FILE} PROPERTIES OUTPUT_LOCATION "${CMAKE_BINARY_DIR}/i18n")

  SET(TS_TARGET "${LANGUAGE}_ts")
  ADD_CUSTOM_TARGET(${TS_TARGET}
    COMMAND ${QT_LUPDATE_EXECUTABLE} ${SEAFILE_TRANSLATE_SOURCES}
        -ts "${PROJECT_SOURCE_DIR}/i18n/seafile_${LANGUAGE}.ts" -no-obsolete
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

  SET(SEAFILE_TS_TARGETS ${SEAFILE_TS_TARGETS} ${TS_TARGET})

ENDFOREACH(LANGUAGE ${LANGUAGES})

ADD_CUSTOM_TARGET(update-ts DEPENDS ${SEAFILE_TS_TARGETS})

QT4_ADD_TRANSLATION(qm_files ${LANGUAGE_TS_FILES})

####################
###### end: translations
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
  ${QT_LIBRARIES}
  ${QT_QTNETWORK_LIBRARY}
  ${OPENSSL_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${JANSSON_LIBRARIES}
  ${LIBSEARPC_LIBRARIES}
  ${LIBCCNET_LIBRARIES}
  ${LIBSEAFILE_LIBRARIES}
  ${EXTRA_LIBS}
)

set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})
add_custom_target(dist
    COMMAND git archive -v --prefix=${ARCHIVE_NAME}/ HEAD
        | gzip > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
