set(twupdaterplugin_SRCS
    twupdaterplugin.cpp
    updaterBackend.cpp
    zypperWrapper.cpp
    )

add_library(twupdaterplugin SHARED ${twupdaterplugin_SRCS})

target_link_libraries(twupdaterplugin Qt5::Core Qt5::Quick Qt5::Qml Qt5::DBus KF5::Plasma KF5::Notifications)

install(TARGETS twupdaterplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/twupdater)

install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/twupdater)
