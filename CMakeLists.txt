CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(seafile-client)
INCLUDE(FindPkgConfig)

SET(CMAKE_BUILD_TYPE Debug)

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
  src/api/api-client.h
  src/api/api-request.h
  src/api/requests.h
  src/rpc/rpc-client.h
  src/ui/main-window.h
  src/ui/init-seafile-dialog.h
  src/ui/login-dialog.h
  src/ui/create-repo-dialog.h
  src/ui/sync-repo-dialog.h
  src/ui/list-account-repos-dialog.h
  src/ui/account-item.h
  src/ui/accounts-view.h
  src/ui/repo-item.h
  src/ui/repos-view.h
  src/ui/tray-icon.h
  src/utils/rsa.h
  src/utils/utils.h
)

# UI FILES
SET(ui_files
  ui/login-dialog.ui
  ui/create-repo.ui
  ui/sync-repo.ui
  ui/init-seafile-dialog.ui
  ui/list-account-repos-dialog.ui
  ui/account-item.ui
  ui/accounts-view.ui
  ui/repo-item.ui
  ui/repos-view.ui
)

# RESOURCES
SET(qrc_files seafile-client.qrc)

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
  src/api/api-client.cpp
  src/api/api-request.cpp
  src/api/requests.cpp
  src/api/server-repo.cpp
  src/rpc/rpc-client.cpp
  src/rpc/local-repo.cpp
  src/ui/main-window.cpp
  src/ui/init-seafile-dialog.cpp
  src/ui/login-dialog.cpp
  src/ui/create-repo-dialog.cpp
  src/ui/sync-repo-dialog.cpp
  src/ui/list-account-repos-dialog.cpp
  src/ui/account-item.cpp
  src/ui/accounts-view.cpp
  src/ui/repo-item.cpp
  src/ui/repos-view.cpp
  src/ui/tray-icon.cpp
  src/utils/rsa.cpp
  src/utils/utils.cpp
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/src
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

ADD_EXECUTABLE(seafile-client
  ${seafile_client_sources}
  ${moc_output}
  ${ui_output}
  ${resources_ouput}
)

TARGET_LINK_LIBRARIES(seafile-client
  ${QT_LIBRARIES}
  ${QT_QTNETWORK_LIBRARY}
  ${OPENSSL_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${JANSSON_LIBRARIES}
  ${LIBSEARPC_LIBRARIES}
  ${LIBCCNET_LIBRARIES}
  ${LIBSEAFILE_LIBRARIES}
)
