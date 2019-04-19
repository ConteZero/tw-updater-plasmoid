cmake_minimum_required(VERSION 3.0)

project(tw-updater-plasmoid VERSION 0.3.2)
add_definitions(-DPROJECT_VERSION="\\\"${PROJECT_VERSION}\\\"")

find_package(ECM 1.3.0 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)
include(FeatureSummary)

find_package(Qt5 CONFIG REQUIRED COMPONENTS Core Quick Qml DBus)
find_package(KF5 REQUIRED COMPONENTS Plasma Notifications)

if(CMAKE_COMPILER_IS_GNUCXX)
  # more aggressive warnings and C++11 support
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
endif()

add_subdirectory(plugin)

install(PROGRAMS zypper-dup-wrapper DESTINATION "/usr/bin")
install(FILES com.contezero.zypper-dup-wrapper.policy DESTINATION /usr/share/polkit-1/actions/)

plasma_install_package(package com.contezero.twupdater)
