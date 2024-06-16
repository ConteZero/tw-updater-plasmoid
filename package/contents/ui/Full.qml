/*
 *   Copyright (C) 2019-2024 by Michele Cherici <contact@contezero.com>
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls

import org.kde.kirigami as Kirigami

import org.kde.plasma.plasmoid
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents


PlasmaExtras.Representation {
    id: full

    focus: true
    anchors.fill: parent

    Layout.minimumHeight: 200
    Layout.minimumWidth: 200
    Layout.maximumWidth: 400

    ColumnLayout {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        width: parent.width

        RowLayout {
            Layout.alignment: Qt.AlignLeft
            spacing: 0
            Layout.fillWidth: true
            visible: headerText != ""
            // Layout.fillHeight: true
            Controls.Label {
                height: Kirigami.Units.iconSizes.medium
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: headerText
            }
        }
        RowLayout {
			id: buttons
			Layout.fillWidth: true
            Layout.fillHeight: true
            visible: buttonInstallUpdatesEnabled || buttonCheckUpdatesEnabled
			// Layout.alignment: Qt.AlignBottom
			PlasmaComponents.Button {
				id: installUpdatesButton
				Layout.fillWidth: true
				visible: buttonInstallUpdatesEnabled
				text: "Install Updates"
//	 			iconName: "update-none"
				onClicked: function () {
					buttonCheckUpdatesEnabled = false
					buttonInstallUpdatesEnabled = false
					installUpdates()
				}
			}
			PlasmaComponents.Button {
				id: checkUpdatesButton
				Layout.fillWidth: true
				visible: buttonCheckUpdatesEnabled
				text: "Check Updates"
// 				iconName: "view-refresh"
				onClicked: function () {
					buttonCheckUpdatesEnabled = false
					buttonInstallUpdatesEnabled = false
					checkUpdates()
				}
			}
		}

    }

    Rectangle {
        id: headerSeparator
        anchors.top: header.bottom
        anchors.topMargin: Kirigami.Units.gridUnit*0.4
        width: parent.width
        height: 1
        color: Kirigami.Theme.textColor
        opacity: 0.25
        visible: buttonInstallUpdatesEnabled || buttonCheckUpdatesEnabled || headerText != ""
    }

    Kirigami.ScrollablePage {
        id: checkScrollArea
        visible: checkScrollAreaVisible
        background: Rectangle {
            anchors.fill: parent
            color: "transparent"
        }
        anchors.top: headerSeparator.bottom
        anchors.bottom: anchorPosVerticalCenter ? parent.verticalCenter : parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        // Layout.fillHeight: true
        // Layout.fillWidth: true
        ListView {
                id: checkView
                // clip: true
                anchors.rightMargin: Kirigami.Units.gridUnit
                model: checkListModel
                // anchors.fill: parent
                // boundsBehavior: Flickable.StopAtBounds
                delegate: CheckDelegate {}
        }
    }

    Kirigami.ScrollablePage {
        id: installScrollArea
        visible: installScrollAreaVisible
        background: Rectangle {
            anchors.fill: parent
            color: "transparent"
        }
        anchors.top: headerSeparator.bottom
        anchors.bottom: anchorPosVerticalCenter ? parent.verticalCenter : parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        // Layout.fillHeight: true
        // Layout.fillWidth: true

        ListView {
            id: installView
            anchors.rightMargin: Kirigami.Units.gridUnit
            model: installListModel
            delegate: InstallDelegate {}
        }
    }

    Kirigami.ScrollablePage {
        id: promptScrollArea
        visible: promptScrollAreaVisible
        background: Rectangle {
            anchors.fill: parent
            color: "transparent"
        }
        anchors.top: anchorPosVerticalCenter ? parent.verticalCenter : headerSeparator.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        // Layout.fillHeight: true
        // Layout.fillWidth: true

        ListView {
            id: promptView
            anchors.rightMargin: Kirigami.Units.gridUnit
            model: promptListModel
            delegate: PromptDelegate {
						onCheckedPromptopt: {
							if (buttonPromptClickable) {
								buttonPromptClickable = false
								chosenPromptOpt = optvalue
								delayButtonPromptTimer.start()
							}
						}
                }
        }
    }


    PlasmaComponents.BusyIndicator {
		id: busyind
        running: busyindRunning
        visible: running
        Layout.preferredHeight: parent.height
        anchors.centerIn: parent
        // Layout.alignment: Qt.AlignCenter
    }

    Component.onCompleted: {
        // print("TW-UPDATER: Full onCompleted " + checkListModel.count)
    }



	Connections {
        target: main

        function onPosViewAtEnd() {
            installView.positionViewAtEnd()
        }
    }


}

