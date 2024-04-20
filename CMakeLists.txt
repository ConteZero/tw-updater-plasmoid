cmake_minimum_required(VERSION 3.0)

project(tw-updater-plasmoid VERSION 0.3.2)
add_definitions(-DPROJECT_VERSION="\\\"${PROJECT_VERSION}\\\"")

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")


find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_AUTOMOC ON)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMQmlModule)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Quick Qml DBus)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Kirigami Notifications)
find_package(Plasma REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
  # more aggressive warnings and C++11 support
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
endif()

add_subdirectory(plugin)

install(PROGRAMS zypper-dup-wrapper DESTINATION "/usr/bin")
install(FILES com.contezero.zypper-dup-wrapper.policy DESTINATION /usr/share/polkit-1/actions/)

plasma_install_package(package com.contezero.twupdater)
