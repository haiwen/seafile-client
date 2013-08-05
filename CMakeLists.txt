cmake_minimum_required(VERSION 2.6)

PROJECT(seafile-client)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})

# MOC FILES
SET(moc_headers src/mainwindow.h)

# UI FILES
SET(ui_files)

# RESOURCES
SET(qrc_files seafile-client.qrc)

# preprocess ui files
QT4_WRAP_UI(ui_output ${ui_files})

# meta object compliation(moc)
QT4_WRAP_CPP(moc_output ${moc_headers})

# resources files
QT4_ADD_RESOURCES(rcc_ouput ${qrc_files})

MESSAGE("moc output: ${moc_output}")
MESSAGE("ui output: ${ui_output}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${QT_INCLUDE_DIR})

SET(seafile_client_sources src/main.cpp src/mainwindow.cpp)

ADD_EXECUTABLE(seafile-client ${seafile_client_sources} ${moc_output} ${ui_output} ${rcc_ouput})

TARGET_LINK_LIBRARIES(seafile-client ${QT_LIBRARIES})
