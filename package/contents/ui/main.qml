/*
 *   Copyright (C) 2019 by Michele Cherici <contact@contezero.com> 
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

import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.private.twupdater 1.0

Item {
	id: mainrep
	
	property string appName: "Tumbleweed Updater"
    property var numupdates: 0
	property bool buttonPromptClickable: true;
	property bool buttonCheckUpdatesEnabled: false
	property bool buttonInstallUpdatesEnabled: false
	property string chosenPromptOpt: ""
	property bool numIndicator: false
	property bool autoagreeLicenses: plasmoid.configuration.autoagreeLicenses
	property bool autoresolveConflicts: plasmoid.configuration.autoresolveConflicts
	property int checkUpdatesInterval: plasmoid.configuration.checkInterval * 1000 * 60 * 60
	
	onCheckUpdatesIntervalChanged: function(){checkUpdatesTimer.interval=checkUpdatesInterval}
	
	Plasmoid.icon: "update-none"
    Plasmoid.switchWidth: units.gridUnit * 10;
    Plasmoid.switchHeight: units.gridUnit * 10;

    Plasmoid.compactRepresentation: Item {
		PlasmaCore.IconItem {
			id: compacticon
			source: "update-none"
			anchors.fill: parent
		}
		Rectangle {
			id: roundrect
			width: (numupdates > 0) ? ((numupdatestext.width * 1.3) > numupdatestext.height ? (numupdatestext.width * 1.3) : numupdatestext.height) : numupdatestext.height
			height: numupdatestext.height
			radius: Math.round(width * 0.5)
			color: "#3daee9"
			visible: (numupdates > 0) || numIndicator
			anchors {
				horizontalCenter: parent.horizontalCenter
				bottom: parent.bottom
			}
		}
		Text {
			id: numupdatestext
			text: numupdates
			font.pointSize: 6
			color: (numupdates > 0) ? "Black" : "transparent"
			anchors.centerIn: roundrect
			visible: roundrect.visible
		}
		MouseArea {
			anchors.fill: parent
			onClicked: {
				plasmoid.expanded = !plasmoid.expanded;
			}
			hoverEnabled: true
		}
    }

    Plasmoid.status: {
		if ((numupdates > 0) || numIndicator || busyind.running) {
            return PlasmaCore.Types.ActiveStatus;
        }
        return PlasmaCore.Types.PassiveStatus;
    }
    
	Connections {
		target: UpdaterBackend
        onCheckCompleted: populateCheckModel(install)
        onInstallCompleted: installCompleted()
		onInstallPrompt: populatePromptModel(promptParams)
		onInstallMessage: populateInstallModel(messageParams)
		onOperationAborted: abortOperation(abortType)
		onInstallResumed: resumeInstall(numPackages)
		onHeaderMessage: changeHeader(messageText)
	}
	
	Component.onCompleted: checkUpdatesFirst()
   
	ListModel {
		id: checkListModel
	}
	
	ListModel {
		id: promptListModel
	}
	
	ListModel {
		id: installListModel
	}
	
    PlasmaExtras.Heading {
        id: header
        level: 4
        wrapMode: Text.WordWrap
        width: parent.width
        text: ""
    }
    
	ColumnLayout {
        spacing: units.smallSpacing
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
            top: header.bottom
        }
        PlasmaComponents.BusyIndicator {
			id: busyind
            running: false
            visible: running
            Layout.preferredHeight: parent.height
            Layout.alignment: Qt.AlignCenter
        }
        PlasmaExtras.ScrollArea {
            id: checkScrollArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: false
            ListView {
                id: checkView
                clip: true
                model: checkListModel
                anchors.fill: parent
                boundsBehavior: Flickable.StopAtBounds
                delegate: CheckDelegate {}
            }
        }
        PlasmaExtras.ScrollArea {
            id: installScrollArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: false
            ListView {
                id: installView
                clip: true
                model: installListModel
                anchors.fill: parent
                boundsBehavior: Flickable.StopAtBounds
                delegate: InstallDelegate {}
            }
        }
        PlasmaExtras.ScrollArea {
            id: promptScrollArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: false
            ListView {
                id: promptView
                clip: true
                model: promptListModel
                anchors.fill: parent
                boundsBehavior: Flickable.StopAtBounds
                delegate: PromptDelegate {
						onCheckedPromptopt: {
							if (buttonPromptClickable) {
								buttonPromptClickable = false;
								chosenPromptOpt = optvalue;
								delayButtonPromptTimer.start();
							}
						}
                }
            }
        }
		RowLayout {
			id: buttons
			Layout.alignment: Qt.AlignBottom
			PlasmaComponents.Button {
				id: installUpdatesButton
				Layout.fillWidth: true
				visible: buttonInstallUpdatesEnabled
				text: "Install Updates"
//	 			iconName: "update-none"
				onClicked: function () {
					buttonCheckUpdatesEnabled = false;
					buttonInstallUpdatesEnabled = false;
					installUpdates();
				}
			}
			PlasmaComponents.Button {
				id: checkUpdatesButton
				Layout.fillWidth: true
				visible: buttonCheckUpdatesEnabled
				text: "Check Updates"
// 				iconName: "view-refresh"
				onClicked: function () {
					buttonCheckUpdatesEnabled = false;
					buttonInstallUpdatesEnabled = false;
					checkUpdates();
				}
			}
		}
	}

	function checkUpdatesFirst() {
		//TODO: try to detct if network is ready instead of a generic timer delayButtonPromptTimer
// 		checkUpdatesTimerFirst.start();
		checkUpdates();
	}
	
	function checkUpdates() {
// 		header.text = "Checking Updates";
		header.text = "";
		installListModel.clear();
		installScrollArea.visible = false;
		checkScrollArea.visible = false;
		promptScrollArea.visible = false;
		busyind.running = true;
		UpdaterBackend.installOptions(autoresolveConflicts, autoagreeLicenses);
		UpdaterBackend.checkUpdates();
	}
	
	function installUpdates() {
// 		if (checkUpdatesTimerFirst.running) checkUpdatesTimerFirst.stop();
// 		header.text = "Installing Updates";
		installListModel.clear();
		installScrollArea.visible = false;
		checkScrollArea.visible = false;
		promptScrollArea.visible = false;
		busyind.running = true;
		UpdaterBackend.installOptions(autoresolveConflicts, autoagreeLicenses);
		UpdaterBackend.installUpdates();
	}

	function abortOperation(aborttype) {
		numIndicator = true;
		busyind.running = false;
		installScrollArea.visible = false;
		promptScrollArea.visible = false;
		checkScrollArea.visible = true;
		buttonCheckUpdatesEnabled = true;
		if (aborttype == 0) {//user abort, no confirm
// 			header.text = "Available Updates: " + numupdates;
			buttonInstallUpdatesEnabled = true;
		} else if (aborttype == -1) {//install resume failed
			buttonInstallUpdatesEnabled = false;
		} else if (aborttype == 1) {//abort check
			buttonInstallUpdatesEnabled = false;
			installScrollArea.visible = false;
			checkScrollArea.visible = false;
		} else if (aborttype == 2) {//abort install
			buttonInstallUpdatesEnabled = false;
			installScrollArea.visible = true;
			checkScrollArea.visible = false;
		} else if (aborttype == 10) {//abort check network
			numIndicator = false;
			buttonInstallUpdatesEnabled = false;
			installScrollArea.visible = false;
			checkScrollArea.visible = false;
		}
	}

	function resumeInstall(npack) {
// 		header.text = "Installing Updates"
		numIndicator = true;
		numupdates = npack;
	}
	
	function changeHeader(hmess) {
		header.text = hmess;
	}
	
	function installCompleted() {
// 		header.text = "Installation Completed";
		header.text = "";
		numupdates = 0;
		numIndicator = false;
		buttonCheckUpdatesEnabled = true;
		buttonInstallUpdatesEnabled = false;
		installView.positionViewAtEnd();
	}

    function populateInstallModel(messageparams) {
		var messageparamsList = messageparams;
		var installType = messageparamsList[0];
		var perc = 0;
		checkScrollArea.visible = false;
		buttonCheckUpdatesEnabled = false;
		buttonInstallUpdatesEnabled = false;
		busyind.running = false;
		installScrollArea.visible = true;
		if ((installType == "m1") || (installType == "m2")) {
			if (installView.count >= 1) {
				installListModel.set(installView.count - 1, {progressvis: false})
			}
			if (installType == "m1") {
				installListModel.append({"installtext": messageparamsList[1], "details": messageparamsList[2], "percentage": 0, "detailsvis": true, "progressvis": true, "infomessage": false})
			} else {
				installListModel.append({"installtext": messageparamsList[1], "details": "", "percentage": 0, "detailsvis": false, "progressvis": false, "infomessage": true})
			}
			installView.positionViewAtEnd();
		} else if (installType == "d") {
			perc = parseInt(messageparamsList[1]);
			if (perc < 0) perc = 0;
			if (messageparamsList[3] == "1") perc = 100;
			if (installListModel.get(installView.count - 1).percentage < perc) {
				installListModel.set(installView.count - 1, {percentage: perc});
			}
		} else if (installType == "p") {
			if ((messageparamsList[2] == "") && (messageparamsList[3] == "")) {
				if (installView.count >= 1) {
					installListModel.set(installView.count - 1, {progressvis: false})
				}
				installListModel.append({"installtext": messageparamsList[1], "details": "", "percentage": 0, "detailsvis": false, "progressvis": true, "infomessage": false})
				installView.positionViewAtEnd();
			} else {
				installListModel.set(installView.count - 1, {installtext: messageparamsList[1]});
				perc = parseInt(messageparamsList[2]);
				if (perc < 0) perc = 0;
				if (messageparamsList[3] == "1") perc = 100;
				if (installListModel.get(installView.count - 1).percentage < perc) {
					installListModel.set(installView.count - 1, {percentage: perc});
				}
			}
		}
// 		print("populateInstallModel " + messageparamsList.length + " " + messageparamsList[0] + " " + messageparamsList[1]);
	}

    function populatePromptModel(promptparams) {
		var promptparamsList = promptparams;
// 		var debugtext = "";
		var promptType = promptparamsList[0];
		if (promptType != "0") {
			checkScrollArea.visible = false;
		}
		promptListModel.clear();
		busyind.running = false;
		installScrollArea.visible = false;
		promptScrollArea.visible = true;
		promptListModel.append({"typeheader": true, "promptopt": promptparamsList[1], "optvalue": ""});
		for (var i = 2; i < promptparamsList.length; i=i+2) {
// 			debugtext = debugtext + " " + promptparamsList[i];
			promptListModel.append({"typeheader": false, "promptopt": promptparamsList[i+1], "optvalue": promptparamsList[i]});
			if ((promptType == "0") && (i == 4)) break;
		}
// 		print("populatePromptModel " + promptparamsList.length + " " + promptparamsList[0] + " " + promptparamsList[1] + " " + debugtext);
	}

	function populateCheckModel(installproc) {
		busyind.running = false;
		installScrollArea.visible = false;
		checkScrollArea.visible = true;
		checkListModel.clear();
		var packageListTmp
		var numPackages = 0;
		packageListTmp = UpdaterBackend.listCheckUpdates();
		var packageList = [];
		for (var i = 0; i < packageListTmp.length; i++)
			packageList[i] = packageListTmp[i].slice();
		for (var i = 0; i < packageListTmp.length; i++) {
			if (packageListTmp[i][0] == 5) {
				for (var j = 0; j < packageList.length; j++) {
					if ((packageList[j][0] > 0) && (packageList[j][0] < 5) && (packageList[j][1] == packageListTmp[i][1])) {
						packageList[j][0] = 0;
						break;
					}
				}
			}
		}
		for (var i = 0; i < packageList.length; i++) {
			if (packageList[i][0] > 0) {
				checkListModel.append({"type": packageList[i][0], "name": packageList[i][1], "version": packageList[i][2], "summary": packageList[i][3]});
				if (packageList[i][0] < 100) numPackages++;
			}
		}
		numupdates = numPackages;
		if (!installproc) {
			buttonCheckUpdatesEnabled = true;
			if (numupdates > 0) {
				if (header.text == "") header.text = "Available Updates: " + numupdates;
				numIndicator = true;
				buttonInstallUpdatesEnabled = true;
			} else {
				numIndicator = false;
			}
		}
//         print("populateCheckModel " + packageList.length + " " + numPackages)
    }

// 	Timer {
// 		id: checkUpdatesTimerFirst
// 		interval: 60000
// 		running: false
// 		repeat: false
// 		triggeredOnStart: false
// 		onTriggered: checkUpdates()
// 	}
	
	Timer {
		id: checkUpdatesTimer
		interval: checkUpdatesInterval
		running: true
		repeat: true
		triggeredOnStart: false
		onTriggered: checkUpdates()
	}
	
	Timer {
		id: delayButtonPromptTimer
		interval: 200
		repeat: false
		onTriggered: {
			promptListModel.clear();
			busyind.running = true;
			checkScrollArea.visible = false;
			promptScrollArea.visible = false;
			buttonPromptClickable = true;
			UpdaterBackend.promptInput(chosenPromptOpt);
		}
	}

}
