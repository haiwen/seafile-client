cmake_minimum_required(VERSION 2.6)

PROJECT(seafile-client)
INCLUDE(FindPkgConfig)

set(CMAKE_BUILD_TYPE Debug)

####################
###### BEGIN: QT configuration
####################
FIND_PACKAGE(Qt4 REQUIRED QtCore QtGui QtNetwork)
INCLUDE(${QT_USE_FILE})

# MOC FILES
SET(moc_headers
  src/account-mgr.h
  src/api/api-client.h
  src/api/api-request.h
  src/api/login-request.h
  src/api/list-repos-request.h
  src/ui/main-window.h
  src/ui/login-dialog.h
  src/ui/account-item.h
  src/ui/account-view.h
  src/ui/list-account-repos-dialog.h
)

# UI FILES
SET(ui_files
  ui/account-item.ui
  ui/account-view.ui
  ui/login-dialog.ui
  ui/list-account-repos-dialog.ui
)

# RESOURCES
SET(qrc_files seafile-client.qrc)

# preprocess ui files
QT4_WRAP_UI(ui_output ${ui_files})

# meta object compliation(moc)
QT4_WRAP_CPP(moc_output ${moc_headers})

# resources files
QT4_ADD_RESOURCES(resources_ouput ${qrc_files})

MESSAGE("moc output: ${moc_output}")
MESSAGE("ui output: ${ui_output}")

####################
###### END: QT configuration
####################

####################
###### BEGIN: JANSSON configuration
####################
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(JANSSON REQUIRED jansson>=2.0)
####################
###### END: JANSSON configuration
####################


# cpp sources
SET(seafile_client_sources
  src/main.cpp
  src/account-mgr.cpp
  src/api/api-client.cpp
  src/api/api-request.cpp
  src/api/login-request.cpp
  src/api/list-repos-request.cpp
  src/api/seaf-repo.cpp
  src/ui/main-window.cpp
  src/ui/login-dialog.cpp
  src/ui/account-item.cpp
  src/ui/account-view.cpp
  src/ui/list-account-repos-dialog.cpp
)

ADD_EXECUTABLE(seafile-client
  ${seafile_client_sources}
  ${moc_output}
  ${ui_output}
  ${resources_ouput}
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/src
  ${QT_INCLUDE_DIR}
  ${QT_QTNETWORK_INCLUDE_DIR}
  ${JANSSON_CFLAGS}
)

TARGET_LINK_LIBRARIES(seafile-client
  ${QT_LIBRARIES}
  ${QT_QTNETWORK_LIBRARY}
  ${JANSSON_LIBRARIES}
)
