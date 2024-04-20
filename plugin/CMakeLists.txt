ecm_add_qml_module(twupdaterplugin URI org.kde.plasma.private.twupdater)

# set(twupdaterplugin_SRCS
#     twupdaterplugin.cpp
#     updaterBackend.cpp
#     zypperWrapper.cpp
#     )

# add_library(twupdaterplugin SHARED ${twupdaterplugin_SRCS})
target_sources(twupdaterplugin PRIVATE 
    twupdaterplugin.cpp
    updaterBackend.cpp
    zypperWrapper.cpp
)

# target_link_libraries(twupdaterplugin Qt6::Core Qt6::Quick Qt6::Qml Qt6::DBus KF6::Kirigami KF6::Notifications)
target_link_libraries(twupdaterplugin PRIVATE Qt6::Core Qt6::Quick Qt6::Qml Qt6::DBus KF6::Kirigami KF6::Notifications)

# install(TARGETS twupdaterplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/twupdater)
# install(TARGETS twupdaterplugin DESTINATION /usr/lib64/qt6/qml/org/kde/plasma/private/twupdater)

# install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/twupdater)
# install(TARGETS qmldir DESTINATION /usr/lib64/qt6/qml/org/kde/plasma/private/twupdater)

# ecm_finalize_qml_module(twupdaterplugin)
ecm_finalize_qml_module(twupdaterplugin DESTINATION ${KDE_INSTALL_QMLDIR})
