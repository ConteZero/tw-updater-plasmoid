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
import QtQuick.Controls
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.private.twupdater


PlasmoidItem {
	id: main
	
	property string appName: "Tumbleweed Updater"
    property var numupdates: 0
	property bool buttonPromptClickable: true;
	property bool buttonCheckUpdatesEnabled: true
	property bool buttonInstallUpdatesEnabled: false
	property string chosenPromptOpt: ""
	property bool numIndicator: false
	property bool autoagreeLicenses: plasmoid.configuration.autoagreeLicenses
	property bool autoresolveConflicts: plasmoid.configuration.autoresolveConflicts
	property bool enableLogging: plasmoid.configuration.enableLogging
	property int checkUpdatesInterval: plasmoid.configuration.checkInterval * 1000 * 60 * 60
	
	property string headerText: ""
	property bool installScrollAreaVisible: false
	property bool checkScrollAreaVisible: false
	property bool promptScrollAreaVisible: false
	property bool busyindRunning: false
	property bool anchorPosVerticalCenter: false
	
	
	signal posViewAtEnd()
	
    compactRepresentation: Compact {}
    fullRepresentation: Full {}
	

    Plasmoid.status: {
		if ((numupdates > 0) || numIndicator || busyindRunning) {
            return PlasmaCore.Types.ActiveStatus;
        }
        return PlasmaCore.Types.PassiveStatus;
    }
    
	onCheckUpdatesIntervalChanged: function() {
		checkUpdatesTimer.interval = checkUpdatesInterval
	}

	Connections {
		target: UpdaterBackend
        function onCheckCompleted(install) { populateCheckModel(install) }
        function onInstallCompleted() { installCompleted() }
		function onInstallPrompt(promptParams) { populatePromptModel(promptParams) }
		function onInstallMessage(messageParams) { populateInstallModel(messageParams) }
		function onOperationAborted(abortType) { abortOperation(abortType) }
		function onInstallResumed(numPackages) { resumeInstall(numPackages) }
		function onHeaderMessage(messageText) { changeHeader(messageText) }
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
	

	function checkUpdatesFirst() {
		//TODO: try to detct if network is ready instead of a generic timer delayButtonPromptTimer
		buttonCheckUpdatesEnabled = false
		checkUpdatesTimerFirst.start()
		// checkUpdates()
	}
	
	function checkUpdates() {
// 		headerText = "Checking Updates"
		headerText = ""
		installListModel.clear()
		installScrollAreaVisible = false
		checkScrollAreaVisible = false
		promptScrollAreaVisible = false
		verifyScrollArea()
		busyindRunning = true
		UpdaterBackend.installOptions(autoresolveConflicts, autoagreeLicenses, enableLogging)
		UpdaterBackend.checkUpdates()
	}
	
	function installUpdates() {
// 		if (checkUpdatesTimerFirst.running) checkUpdatesTimerFirst.stop()
// 		headerText = "Installing Updates"
		installListModel.clear()
		installScrollAreaVisible = false
		checkScrollAreaVisible = false
		promptScrollAreaVisible = false
		verifyScrollArea()
		busyindRunning = true
		UpdaterBackend.installOptions(autoresolveConflicts, autoagreeLicenses, enableLogging)
		UpdaterBackend.installUpdates()
	}

	function abortOperation(aborttype) {
		numIndicator = true
		busyindRunning = false
		installScrollAreaVisible = false
		promptScrollAreaVisible = false
		checkScrollAreaVisible = true
		buttonCheckUpdatesEnabled = true
		if (aborttype == 0) {//user abort, no confirm
// 			headerText = "Available Updates: " + numupdates
			buttonInstallUpdatesEnabled = true
		} else if (aborttype == -1) {//install resume failed
			buttonInstallUpdatesEnabled = false
		} else if (aborttype == 1) {//abort check
			buttonInstallUpdatesEnabled = false
			installScrollAreaVisible = false
			checkScrollAreaVisible = false
		} else if (aborttype == 2) {//abort install
			buttonInstallUpdatesEnabled = false
			installScrollAreaVisible = true
			checkScrollAreaVisible = false
		} else if (aborttype == 10) {//abort check network
			numIndicator = false
			buttonInstallUpdatesEnabled = false
			installScrollAreaVisible = false
			checkScrollAreaVisible = false
		}
		verifyScrollArea()
	}

	function resumeInstall(npack) {
// 		headerText = "Installing Updates"
		numIndicator = true
		numupdates = npack
	}
	
	function changeHeader(hmess) {
		headerText = hmess
	}
	
	function installCompleted() {
// 		headerText = "Installation Completed";
		headerText = ""
		numupdates = 0
		numIndicator = false
		buttonCheckUpdatesEnabled = true
		buttonInstallUpdatesEnabled = false
		posViewAtEnd()
	}

    function populateInstallModel(messageparams) {
		var messageparamsList = messageparams
		var installType = messageparamsList[0]
		var perc = 0
		checkScrollAreaVisible = false
		buttonCheckUpdatesEnabled = false
		buttonInstallUpdatesEnabled = false
		busyindRunning = false
		installScrollAreaVisible = true
		verifyScrollArea()
		var lastIndex = installListModel.count -1
		if ((installType == "m1") || (installType == "m2")) {
			if (lastIndex >= 0) {
				installListModel.set(lastIndex, {progressvis: false})
			}
			if (installType == "m1") {
				installListModel.append({"installtext": messageparamsList[1], "details": messageparamsList[2], "percentage": 0, "detailsvis": true, "progressvis": true, "infomessage": false})
			} else {
				installListModel.append({"installtext": messageparamsList[1], "details": "", "percentage": 0, "detailsvis": false, "progressvis": false, "infomessage": true})
			}
			posViewAtEnd()
		} else if (installType == "d") {
			if (lastIndex >= 0) {
				perc = parseInt(messageparamsList[1])
				if (perc < 0) perc = 0
				if (messageparamsList[3] == "1") perc = 100
				if (installListModel.get(lastIndex).percentage < perc) {
					installListModel.set(lastIndex, {percentage: perc})
				}
			}
		} else if (installType == "p") {
			if ((messageparamsList[2] == "") && (messageparamsList[3] == "")) {
				if (lastIndex >= 0) {
					installListModel.set(lastIndex, {progressvis: false})
				}
				installListModel.append({"installtext": messageparamsList[1], "details": "", "percentage": 0, "detailsvis": false, "progressvis": true, "infomessage": false})
				posViewAtEnd()
			} else {
				if (lastIndex >= 0) {
					installListModel.set(lastIndex, {installtext: messageparamsList[1]})
					perc = parseInt(messageparamsList[2])
					if (perc < 0) perc = 0
					if (messageparamsList[3] == "1") perc = 100
					if (installListModel.get(lastIndex).percentage < perc) {
						installListModel.set(lastIndex, {percentage: perc})
					}
				}
			}
		}
// 		print("populateInstallModel " + messageparamsList.length + " " + messageparamsList[0] + " " + messageparamsList[1]);
	}

    function populatePromptModel(promptparams) {
		var promptparamsList = promptparams
// 		var debugtext = ""
		var promptType = promptparamsList[0]
		if (promptType != "0") {
			checkScrollAreaVisible = false
		}
		promptListModel.clear()
		busyindRunning = false
		installScrollAreaVisible = false
		promptScrollAreaVisible = true
		verifyScrollArea()
		promptListModel.append({"typeheader": true, "promptopt": promptparamsList[1], "optvalue": ""})
		for (var i = 2; i < promptparamsList.length; i=i+2) {
// 			debugtext = debugtext + " " + promptparamsList[i]
			promptListModel.append({"typeheader": false, "promptopt": promptparamsList[i+1], "optvalue": promptparamsList[i]})
			if ((promptType == "0") && (i == 4)) break
		}
// 		print("populatePromptModel " + promptparamsList.length + " " + promptparamsList[0] + " " + promptparamsList[1] + " " + debugtext)
	}

	function populateCheckModel(installproc) {
		busyindRunning = false
		installScrollAreaVisible = false
		checkScrollAreaVisible = true
		verifyScrollArea()
		checkListModel.clear()
		var packageListTmp
		var numPackages = 0
		packageListTmp = UpdaterBackend.listCheckUpdates()
		var packageList = []
		for (var i = 0; i < packageListTmp.length; i++)
			packageList[i] = packageListTmp[i].slice()
		for (var i = 0; i < packageListTmp.length; i++) {
			if (packageListTmp[i][0] == 5) {
				for (var j = 0; j < packageList.length; j++) {
					if ((packageList[j][0] > 0) && (packageList[j][0] < 5) && (packageList[j][1] == packageListTmp[i][1])) {
						packageList[j][0] = 0
						break
					}
				}
			}
		}
		for (var i = 0; i < packageList.length; i++) {
			if (packageList[i][0] > 0) {
				// checkListModel.append({"type": packageList[i][0], "name": packageList[i][1], "version": packageList[i][2], "summary": packageList[i][3]})
				checkListModel.append({"type": packageList[i][0], "name": packageList[i][1], "versionold": packageList[i][2], "version": packageList[i][3], "summary": packageList[i][4]})
				if (packageList[i][0] < 100) numPackages++
			}
		}
		numupdates = numPackages
		if (!installproc) {
			buttonCheckUpdatesEnabled = true;
			if (numupdates > 0) {
				if (headerText == "") headerText = "Available Updates: " + numupdates
				numIndicator = true
				buttonInstallUpdatesEnabled = true
			} else {
				numIndicator = false;
			}
		}
//         print("populateCheckModel " + packageList.length + " " + numPackages)
    }
    
    function verifyScrollArea() {
		var countVis = 0
		if (checkScrollAreaVisible) countVis++
		if (installScrollAreaVisible) countVis++
		if (promptScrollAreaVisible) countVis++
		anchorPosVerticalCenter = false
		if (countVis > 1) anchorPosVerticalCenter = true
	}

	Timer {
		id: checkUpdatesTimerFirst
		interval: 10000
		running: false
		repeat: false
		triggeredOnStart: false
		onTriggered: checkUpdates()
	}
	
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
			promptListModel.clear()
			busyindRunning = true
			checkScrollAreaVisible = false
			promptScrollAreaVisible = false
			verifyScrollArea()
			buttonPromptClickable = true
			UpdaterBackend.promptInput(chosenPromptOpt)
		}
	}

}
